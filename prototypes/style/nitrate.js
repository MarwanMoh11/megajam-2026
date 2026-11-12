// Nitrate look for the three.js prototypes (locked Thu Nov 12). Spec: docs/ART_STYLE.md. Working demo: nitrate-three.html.
// Built against three@0.160.0. For a one-file prototype, paste this module in and keep the exports' names.
//
// Use:
//   const grade = nitrateComposer(renderer, scene, camera, 'silver');   // then grade.composer.render() each frame
//   mesh.material = nitrateMaterial('hero');                             // or applyNitrate(gltf.scene, 'hero')
//   dangerMesh.material = stencil('red');                                // the only colour on screen
//   const film = filmOverlay(document.querySelector('#picture'));       // grain, flicker, dust, gate, cut, iris
//   grade.setTone('amber'); film.splice();                               // a cut
import * as THREE from 'three';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';

export const PALETTE = {
  ink: '#0D0C0B', silver: '#7A756D', bone: '#C9C2B4', paper: '#F1EADB', // text is Paper on Ink
  red: '#D3301F',  // danger, now. Nothing else
  gold: '#E9B949', // hero accent, bonus, close shave, the Director's Cut
};

// One tone per location: [dark, mid, light]. K: night = Rooftop, amber = Mill, green = Railway. L and menus: silver.
export const TONES = {
  night: ['#0B1420', '#5F7387', '#EEF0EA'],
  amber: ['#1A1108', '#9B7A4D', '#F6EBD3'],
  green: ['#0E1712', '#6C806D', '#EEF0E4'],
  silver: ['#0D0C0B', '#7A756D', '#F1EADB'],
};

// The value ladder as sRGB greys (0-255): the hero is the lightest thing on screen, heavies the darkest.
export const VALUES = {
  hero: [218, 237], helpless: [196, 225], set: [89, 149], floor: [97, 129], backdrop: [149, 179], heavy: [25, 48],
};

const srgbVec = (hex) => {
  const n = parseInt(hex.slice(1), 16);
  return new THREE.Vector3((n >> 16 & 255) / 255, (n >> 8 & 255) / 255, (n & 255) / 255);
};

// Post pass: tones every neutral pixel onto the location's three stops, keeps saturated pixels (red, gold) as they are.
// Runs after OutputPass, on display values, exactly like prototypes/style/make_luts.py.
export const NitrateGradeShader = {
  uniforms: {
    tDiffuse: { value: null },
    uDark: { value: srgbVec(TONES.silver[0]) }, uMid: { value: srgbVec(TONES.silver[1]) }, uLight: { value: srgbVec(TONES.silver[2]) },
    uCurve: { value: 0.35 },  // S-curve contrast
    uAmount: { value: 1.0 },  // 0 = raw render, for comparison
  },
  vertexShader: `varying vec2 vUv; void main(){ vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }`,
  fragmentShader: `
    uniform sampler2D tDiffuse; uniform vec3 uDark, uMid, uLight; uniform float uCurve, uAmount; varying vec2 vUv;
    void main(){
      vec3 c = texture2D(tDiffuse, vUv).rgb;
      float v = clamp(dot(c, vec3(0.2126, 0.7152, 0.0722)), 0.0, 1.0);
      float s = v < 0.5 ? 2.0 * v * v : 1.0 - 2.0 * (1.0 - v) * (1.0 - v);
      v = mix(v, s, uCurve);
      vec3 toned = v < 0.5 ? mix(uDark, uMid, v / 0.5) : mix(uMid, uLight, (v - 0.5) / 0.5);
      float mx = max(c.r, max(c.g, c.b)), mn = min(c.r, min(c.g, c.b));
      float sat = mx > 0.0 ? (mx - mn) / mx : 0.0;
      vec3 graded = mix(toned, c, smoothstep(0.30, 0.55, sat));
      gl_FragColor = vec4(mix(c, graded, uAmount), 1.0);
    }`,
};

export function nitrateComposer(renderer, scene, camera, tone = 'silver') {
  renderer.toneMapping = THREE.NoToneMapping; // keeps the value ladder predictable
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  const composer = new EffectComposer(renderer);
  composer.addPass(new RenderPass(scene, camera));
  composer.addPass(new OutputPass());
  const pass = new ShaderPass(NitrateGradeShader);
  composer.addPass(pass);
  const setTone = (name) => {
    const [d, m, l] = TONES[name];
    pass.uniforms.uDark.value.copy(srgbVec(d)); pass.uniforms.uMid.value.copy(srgbVec(m)); pass.uniforms.uLight.value.copy(srgbVec(l));
  };
  setTone(tone);
  return { composer, pass, setTone, setSize: (w, h) => { renderer.setSize(w, h, false); composer.setSize(w, h); } };
}

// Lit material for a role on the value ladder. With a texture (an AI asset), the texture's colour is dropped:
// its brightness is remapped into the role's band, so mismatched assets come out as one family.
export function nitrateMaterial(role, { map = null, contrast = 1.0, roughness = 0.75 } = {}) {
  const [lo, hi] = VALUES[role];
  const mat = new THREE.MeshStandardMaterial({ roughness, metalness: 0, map });
  if (!map) {
    const g = Math.round((lo + hi) / 2);
    mat.color.setRGB(g / 255, g / 255, g / 255, THREE.SRGBColorSpace);
    return mat;
  }
  mat.onBeforeCompile = (shader) => {
    shader.uniforms.nLo = { value: lo / 255 }; shader.uniforms.nHi = { value: hi / 255 }; shader.uniforms.nContrast = { value: contrast };
    shader.fragmentShader = 'uniform float nLo, nHi, nContrast;\n' + shader.fragmentShader.replace('#include <map_fragment>', `
      #ifdef USE_MAP
        vec4 nTex = texture2D(map, vMapUv);
        float nL = pow(max(dot(nTex.rgb, vec3(0.2126, 0.7152, 0.0722)), 0.0), 1.0 / 2.2);
        diffuseColor.rgb = vec3(pow(mix(nLo, nHi, clamp(pow(nL, nContrast), 0.0, 1.0)), 2.2));
        diffuseColor.a *= nTex.a;
      #endif`);
  };
  mat.customProgramCacheKey = () => `nitrate-${role}-${contrast}`;
  return mat;
}

// Re-skin a loaded model (rigged or not): every mesh gets the role's material, keeping its texture for detail only.
export function applyNitrate(root, role, opts = {}) {
  root.traverse((o) => {
    if (!o.isMesh) return;
    const old = o.material;
    o.material = nitrateMaterial(role, { ...opts, map: old && old.map ? old.map : null });
    o.castShadow = o.receiveShadow = true;
    if (old && old.dispose) old.dispose();
  });
  return root;
}

// Unlit stencil colour: stays fully saturated through the grade. Red = danger only, gold = hero accent and payoffs.
export const stencil = (which = 'red') => new THREE.MeshBasicMaterial({ color: PALETTE[which] });

// Single hard key light, as in the guide: white, one shadow, low fill. Tune intensity until lit floors read 95-130.
export function nitrateLights(scene, { dir = [-0.4, 1, 0.35], intensity = 2.6, fill = 0.55, target = null } = {}) {
  const hemi = new THREE.HemisphereLight(0xffffff, 0x202020, fill);
  const key = new THREE.DirectionalLight(0xffffff, intensity);
  key.position.set(...dir).multiplyScalar(30);
  key.castShadow = true;
  key.shadow.mapSize.set(2048, 2048);
  key.shadow.bias = -0.0005;
  Object.assign(key.shadow.camera, { left: -20, right: 20, top: 20, bottom: -20, near: 1, far: 80 });
  if (target) key.target = target;
  scene.add(hemi, key, key.target);
  return { hemi, key };
}

export const FONTS_HREF = 'https://fonts.googleapis.com/css2?family=IM+Fell+English:ital@0;1&family=Josefin+Sans:wght@600;700&family=Limelight&display=swap';
export function loadFonts() {
  const l = document.createElement('link'); l.rel = 'stylesheet'; l.href = FONTS_HREF; document.head.appendChild(l);
}

// Film layers over the picture element (the 4:3 box that holds the canvas). Everything ticks at 24 fps.
// Gameplay and HUD stay at full frame rate; put HUD elements above this overlay (z-index > 10).
export function filmOverlay(picture, { grain = 0.10, flicker = 0.04, dust = true } = {}) {
  Object.assign(picture.style, { position: 'relative', overflow: 'hidden', borderRadius: '2.5vh', background: PALETTE.ink });
  const layer = (css) => {
    const d = document.createElement('div');
    d.style.cssText = 'position:absolute;inset:0;pointer-events:none;z-index:10;' + css;
    picture.appendChild(d); return d;
  };
  const noise = document.createElement('canvas'); noise.width = noise.height = 256;
  const nc = noise.getContext('2d'); const img = nc.createImageData(256, 256);
  for (let i = 0; i < img.data.length; i += 4) { const v = 128 + (Math.random() - 0.5) * 255; img.data[i] = img.data[i + 1] = img.data[i + 2] = v; img.data[i + 3] = 255; }
  nc.putImageData(img, 0, 0);
  const vignette = layer('background:radial-gradient(ellipse at center, transparent 55%, rgba(13,12,11,.55) 100%);');
  const grainL = layer(`background-image:url(${noise.toDataURL()});background-size:256px 256px;opacity:${grain};mix-blend-mode:overlay;`);
  const flick = layer('background:#000;opacity:0;');
  const specks = layer('');
  const flash = layer(`background:${PALETTE.paper};opacity:0;`);
  const bar = layer(`top:auto;height:0;background:${PALETTE.ink};`);
  const iris = layer('opacity:0;');
  let raf = 0, lastTick = -1, nextScratch = 8, splicing = -1, irisAnim = null;
  const tick = (now) => {
    raf = requestAnimationFrame(tick);
    const t = now / 1000, frame = Math.floor(t * 24);
    if (irisAnim) {
      const k = Math.min(1, (now - irisAnim.start) / irisAnim.ms), e = 1 - Math.pow(1 - k, 3);
      const r = (1 - e) * 120;
      iris.style.opacity = 1;
      iris.style.background = `radial-gradient(circle at ${irisAnim.x * 100}% ${irisAnim.y * 100}%, transparent ${r}%, ${PALETTE.ink} ${r + 0.4}%)`;
      if (k >= 1 && irisAnim.done) { const d = irisAnim.done; irisAnim.done = null; d(); }
    }
    if (frame === lastTick) return;
    lastTick = frame;
    grainL.style.backgroundPosition = `${Math.random() * 256 | 0}px ${Math.random() * 256 | 0}px`;
    flick.style.opacity = (Math.random() * flicker).toFixed(3);
    specks.innerHTML = '';
    if (dust && Math.random() < 2 / 24) {
      const s = 2 + Math.random() * 4, light = Math.random() < 0.5;
      specks.innerHTML = `<i style="position:absolute;left:${Math.random() * 100}%;top:${Math.random() * 100}%;width:${s}px;height:${s}px;border-radius:50%;background:${light ? '#eee' : '#111'};opacity:.7"></i>`;
    }
    if (dust && t > nextScratch) {
      specks.innerHTML += `<i style="position:absolute;left:${10 + Math.random() * 80}%;top:0;bottom:0;width:1px;background:rgba(240,235,220,.35)"></i>`;
      if (t > nextScratch + 2 / 24) nextScratch = t + 6 + Math.random() * 4;
    }
    if (splicing >= 0) {
      flash.style.opacity = splicing === 0 ? 0.85 : 0;
      bar.style.height = ['2%', '4%', '2%'][splicing] || '0';
      splicing = splicing >= 2 ? -1 : splicing + 1;
    } else { flash.style.opacity = 0; bar.style.height = '0'; }
  };
  raf = requestAnimationFrame(tick);
  return {
    splice() { splicing = 0; lastTick = -1; },                 // K: call on the frame of the cut
    iris(x = 0.5, y = 0.5, ms = 450, done = null) {            // L: end of take; x, y = star position in 0-1 of the picture
      irisAnim = { x, y, ms, done, start: performance.now() };
    },
    clearIris() { irisAnim = null; iris.style.opacity = 0; },
    destroy() { cancelAnimationFrame(raf); [vignette, grainL, flick, specks, flash, bar, iris].forEach((d) => d.remove()); },
  };
}
