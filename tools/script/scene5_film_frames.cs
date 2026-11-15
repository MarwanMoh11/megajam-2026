// Scene 5: the frames of LE SOLEIL NOIR, the film on the auditorium screen (QA Nov 15 evening: "add a looping film
// there that's longer, maybe some fun Easter eggs"). Owner: s5-auditorium.
//
// Draws every shot as one cell of a 4096 x 4096 atlas (4 columns x 9 rows of 1024 x 455, the screen's 2.33:1 near
// enough) in black and white: titles and intertitles in the game's own fonts (Limelight, IM Fell English), drawn
// silhouette scenes (Paris under a black sun, the woman at the blinds, the night drive, the man with the can on the
// roof, the duel on the stair, the eclipse), two of the building's own stills (tools/build_pictures.py), and the
// Easter eggs. M_S5_Film (tools/script/scene5_film.py) plays the cells in order, ShotSeconds each, and loops.
//
// Build and run (no editor, .NET Framework's compiler):
//   C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe /nologo /r:System.Drawing.dll /out:<tmp>\s5film.exe tools\script\scene5_film_frames.cs
//   <tmp>\s5film.exe C:\Jam\megajam C:\Jam\Builds\s5\film
// Writes T_S5_FilmAtlas.png (the atlas), contact.png (a half-size look) and shots.txt (the cell list).
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.IO;

public static class S5Film
{
	const int AtlasW = 4096, AtlasH = 4096, Cols = 4, Rows = 9;
	const int CW = AtlasW / Cols, CH = AtlasH / Rows;	// 1024 x 455
	static PrivateFontCollection Fonts = new PrivateFontCollection();
	static FontFamily Deco, Fell, FellItalic, Josefin;
	static string Repo, Out;
	static Random Rng = new Random(1974);

	delegate void Draw(Graphics g);

	public static int Main(string[] args)
	{
		Repo = args.Length > 0 ? args[0] : @"C:\Jam\megajam";
		Out = args.Length > 1 ? args[1] : @"C:\Jam\Builds\s5\film";
		Directory.CreateDirectory(Out);
		string F = Path.Combine(Repo, @"Reel\SourceArt\Fonts");
		Fonts.AddFontFile(Path.Combine(F, @"Limelight\Limelight-Regular.ttf"));
		Fonts.AddFontFile(Path.Combine(F, @"IMFellEnglish\IMFeENrm28P.ttf"));
		Fonts.AddFontFile(Path.Combine(F, @"IMFellEnglish\IMFeENit28P.ttf"));
		Fonts.AddFontFile(Path.Combine(F, @"JosefinSans\JosefinSans-SemiBold.ttf"));
		foreach (FontFamily f in Fonts.Families)
		{
			if (f.Name.Contains("Limelight")) Deco = f;
			else if (f.Name.Contains("Josefin")) Josefin = f;
			else if (f.Name.ToUpperInvariant().Contains("FELL")) { if (f.IsStyleAvailable(FontStyle.Italic)) FellItalic = f; Fell = f; }
		}
		if (FellItalic == null) FellItalic = Fell;
		Console.WriteLine("fonts: " + (Deco != null ? Deco.Name : "-") + ", " + (Fell != null ? Fell.Name : "-") + ", " + (Josefin != null ? Josefin.Name : "-"));

		string Pics = @"C:\Jam\Builds\qa\pictures\src";
		var Shots = new List<KeyValuePair<string, Draw>>();
		Action<string, Draw> Add = (n, d) => Shots.Add(new KeyValuePair<string, Draw>(n, d));

		Add("studio", g => Studio(g));
		Add("director", g => Card(g, new[] { "un film de", "ÉTIENNE VAUCLAIR" }, new[] { 0.34f, 0.62f }, new[] { 34f, 64f }, new[] { 'i', 'd' }));
		Add("title", g => TitleCard(g));
		Add("title_hold", g => TitleCard(g));
		Add("starring", g => Card(g, new[] { "avec", "HÉLÈNE MARCHETTI", "JACQUES DUVAL  ·  ONDINE ROUX" }, new[] { 0.28f, 0.50f, 0.70f }, new[] { 30f, 58f, 38f }, new[] { 'i', 'd', 'd' }));
		Add("egg_max", g => Card(g, new[] { "and introducing", "MAX", "as The Voice in the Van" }, new[] { 0.27f, 0.52f, 0.76f }, new[] { 30f, 92f, 34f }, new[] { 'i', 'd', 'i' }));
		Add("paris_inter", g => Inter(g, "PARIS, 1974.", "At noon, the sun went black."));
		Add("paris", g => Paris(g));
		Add("blinds", g => Blinds(g));
		Add("one_print", g => Inter(g, "« Il n'en existe qu'une seule copie. »", "There is only one print."));
		Add("st_street", g => Still(g, Path.Combine(Pics, "st_street.png")));
		Add("drive", g => Drive(g));
		Add("egg_door", g => Card(g, new[] { "Psst.", "The oak door is on your right.", "— the Projectionist" }, new[] { 0.30f, 0.52f, 0.72f }, new[] { 40f, 46f, 28f }, new[] { 'i', 'r', 'i' }));
		Add("roof", g => Roof(g));
		Add("projector", g => Projector(g));
		Add("intermission", g => Card(g, new[] { "INTERMISSION", "Kindly do not steal the film." }, new[] { 0.42f, 0.66f }, new[] { 84f, 40f }, new[] { 'd', 'i' }));
		Add("egg_cole", g => Mirrored(g));
		Add("stair", g => Stair(g));
		Add("st_hall", g => Still(g, Path.Combine(Pics, "st_hall.png")));
		Add("nobody", g => Inter(g, "« Personne ne vole le soleil. »", "Nobody steals the sun."));
		Add("eclipse", g => Eclipse(g));
		Add("egg_reels", g => Card(g, new[] { "No reels were harmed", "in the making of this picture.", "(Yet.)" }, new[] { 0.32f, 0.50f, 0.72f }, new[] { 48f, 48f, 34f }, new[] { 'r', 'r', 'i' }));
		Add("vault", g => Vault(g));
		Add("egg_cut", g => Card(g, new[] { "THE DIRECTOR'S CUT", "every scene the studio cut, put back in", "(including this one)" }, new[] { 0.36f, 0.60f, 0.76f }, new[] { 70f, 36f, 28f }, new[] { 'd', 'i', 'i' }));
		Add("fin", g => Fin(g));
		Add("egg_jam", g => Card(g, new[] { "An", "EPIC MEGAJAM 2026", "picture   ·   thank you for watching   ·   it loops" }, new[] { 0.30f, 0.50f, 0.72f }, new[] { 30f, 72f, 28f }, new[] { 'i', 'd', 'j' }));

		using (var Atlas = new Bitmap(AtlasW, AtlasH, PixelFormat.Format24bppRgb))
		using (var A = Graphics.FromImage(Atlas))
		{
			A.Clear(Color.Black);
			var List = new List<string>();
			for (int i = 0; i < Shots.Count; i++)
			{
				using (var Cell = new Bitmap(CW, CH, PixelFormat.Format24bppRgb))
				{
					using (var g = Graphics.FromImage(Cell))
					{
						g.SmoothingMode = SmoothingMode.AntiAlias;
						g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
						g.InterpolationMode = InterpolationMode.HighQualityBicubic;
						g.Clear(Color.Black);
						Shots[i].Value(g);
					}
					A.DrawImageUnscaled(Cell, (i % Cols) * CW, (i / Cols) * CH);
				}
				List.Add(i + " " + Shots[i].Key);
			}
			Atlas.Save(Path.Combine(Out, "T_S5_FilmAtlas.png"), ImageFormat.Png);
			using (var C = new Bitmap(AtlasW / 2, AtlasH / 2))
			using (var cg = Graphics.FromImage(C))
			{
				cg.InterpolationMode = InterpolationMode.HighQualityBicubic;
				cg.DrawImage(Atlas, 0, 0, AtlasW / 2, AtlasH / 2);
				C.Save(Path.Combine(Out, "contact.png"), ImageFormat.Png);
			}
			File.WriteAllLines(Path.Combine(Out, "shots.txt"), List.ToArray());
			Console.WriteLine("shots: " + Shots.Count + " -> " + Path.Combine(Out, "T_S5_FilmAtlas.png"));
		}
		return 0;
	}

	// ---- type ---------------------------------------------------------------------------------------------------------

	static Font MakeFont(char kind, float px)
	{
		switch (kind)
		{
			case 'd': return new Font(Deco ?? FontFamily.GenericSerif, px, FontStyle.Regular, GraphicsUnit.Pixel);
			case 'i': return new Font(FellItalic ?? FontFamily.GenericSerif, px, FellItalic != null && FellItalic.IsStyleAvailable(FontStyle.Italic) ? FontStyle.Italic : FontStyle.Regular, GraphicsUnit.Pixel);
			case 'j': return new Font(Josefin ?? FontFamily.GenericSansSerif, px, FontStyle.Regular, GraphicsUnit.Pixel);
			default: return new Font(Fell ?? FontFamily.GenericSerif, px, FontStyle.Regular, GraphicsUnit.Pixel);
		}
	}

	static void Text(Graphics g, string s, float cx, float cy, char kind, float px, Color c)
	{
		using (var f = MakeFont(kind, px))
		using (var b = new SolidBrush(c))
		{
			var sf = new StringFormat { Alignment = StringAlignment.Center, LineAlignment = StringAlignment.Center };
			g.DrawString(s, f, b, new RectangleF(cx - CW, cy - px, CW * 2, px * 2), sf);
		}
	}

	static void Border(Graphics g, int grey)
	{
		using (var p = new Pen(Color.FromArgb(grey, grey, grey), 3f))
		{
			g.DrawRectangle(p, 40, 30, CW - 80, CH - 60);
			p.Width = 1.2f;
			g.DrawRectangle(p, 52, 42, CW - 104, CH - 84);
			// deco corners
			foreach (var pt in new[] { new PointF(40, 30), new PointF(CW - 40, 30), new PointF(40, CH - 30), new PointF(CW - 40, CH - 30) })
			{
				g.FillEllipse(new SolidBrush(Color.FromArgb(grey, grey, grey)), pt.X - 7, pt.Y - 7, 14, 14);
			}
		}
	}

	static void Card(Graphics g, string[] lines, float[] ys, float[] px, char[] kinds)
	{
		Backdrop(g, 14, 30);
		Border(g, 150);
		for (int i = 0; i < lines.Length; i++)
		{
			Text(g, lines[i], CW / 2f, CH * ys[i], kinds[i], px[i], kinds[i] == 'd' ? Color.FromArgb(240, 236, 226) : Color.FromArgb(205, 200, 190));
		}
	}

	static void Inter(Graphics g, string fr, string en)
	{
		Backdrop(g, 10, 22);
		Border(g, 130);
		Text(g, fr, CW / 2f, CH * 0.43f, 'r', 50f, Color.FromArgb(236, 232, 222));
		using (var p = new Pen(Color.FromArgb(150, 150, 150), 1.5f)) g.DrawLine(p, CW * 0.38f, CH * 0.57f, CW * 0.62f, CH * 0.57f);
		Text(g, en, CW / 2f, CH * 0.68f, 'i', 36f, Color.FromArgb(190, 186, 176));
	}

	static void Backdrop(Graphics g, int edge, int middle)
	{
		using (var path = new GraphicsPath())
		{
			path.AddEllipse(-CW * 0.3f, -CH * 0.6f, CW * 1.6f, CH * 2.2f);
			using (var b = new PathGradientBrush(path) { CenterColor = Color.FromArgb(middle, middle, middle), SurroundColors = new[] { Color.FromArgb(edge, edge, edge) } })
			{
				g.FillRectangle(new SolidBrush(Color.FromArgb(edge, edge, edge)), 0, 0, CW, CH);
				g.FillPath(b, path);
			}
		}
	}

	// ---- the titles -----------------------------------------------------------------------------------------------------

	static void Studio(Graphics g)
	{
		Backdrop(g, 8, 26);
		// a sunburst behind the studio's name
		float cx = CW / 2f, cy = CH * 0.46f;
		for (int i = 0; i < 48; i++)
		{
			double a = i * Math.PI * 2 / 48;
			int v = i % 2 == 0 ? 70 : 40;
			using (var p = new Pen(Color.FromArgb(v, v, v), 2f))
				g.DrawLine(p, cx + (float)Math.Cos(a) * 70, cy + (float)Math.Sin(a) * 70, cx + (float)Math.Cos(a) * 560, cy + (float)Math.Sin(a) * 560);
		}
		g.FillEllipse(new SolidBrush(Color.FromArgb(225, 222, 212)), cx - 62, cy - 62, 124, 124);
		g.FillEllipse(new SolidBrush(Color.FromArgb(12, 12, 12)), cx - 52, cy - 52, 104, 104);
		Text(g, "LF", cx, cy + 2, 'd', 54f, Color.FromArgb(230, 226, 216));
		Text(g, "LUMIÈRE FILMS", cx, CH * 0.80f, 'd', 52f, Color.FromArgb(236, 232, 222));
		Text(g, "présente", cx, CH * 0.92f, 'i', 26f, Color.FromArgb(180, 176, 166));
	}

	static void BlackSun(Graphics g, float cx, float cy, float r, int rays)
	{
		for (int i = 0; i < rays; i++)
		{
			double a = i * Math.PI * 2 / rays + 0.05;
			float len = r * (1.5f + 0.9f * (float)Rng.NextDouble());
			int v = 120 + Rng.Next(100);
			using (var p = new Pen(Color.FromArgb(v, v, v), 1.5f + (float)Rng.NextDouble() * 2.5f))
				g.DrawLine(p, cx + (float)Math.Cos(a) * r * 1.02f, cy + (float)Math.Sin(a) * r * 1.02f, cx + (float)Math.Cos(a) * len, cy + (float)Math.Sin(a) * len);
		}
		using (var path = new GraphicsPath())
		{
			path.AddEllipse(cx - r * 1.35f, cy - r * 1.35f, r * 2.7f, r * 2.7f);
			using (var b = new PathGradientBrush(path) { CenterColor = Color.FromArgb(255, 250, 250, 245), SurroundColors = new[] { Color.FromArgb(0, 0, 0, 0) } })
				g.FillPath(b, path);
		}
		g.FillEllipse(new SolidBrush(Color.FromArgb(4, 4, 4)), cx - r, cy - r, r * 2, r * 2);
	}

	static void TitleCard(Graphics g)
	{
		Backdrop(g, 6, 20);
		var save = Rng; Rng = new Random(7);
		BlackSun(g, CW / 2f, CH * 0.36f, 74f, 64);
		Rng = save;
		Text(g, "LE SOLEIL NOIR", CW / 2f, CH * 0.72f, 'd', 104f, Color.FromArgb(244, 240, 230));
		Text(g, "1974", CW / 2f, CH * 0.90f, 'j', 26f, Color.FromArgb(170, 166, 156));
	}

	static void Fin(Graphics g)
	{
		g.Clear(Color.Black);
		using (var path = new GraphicsPath())
		{
			path.AddEllipse(CW / 2f - 260, CH / 2f - 190, 520, 380);
			using (var b = new PathGradientBrush(path) { CenterColor = Color.FromArgb(40, 40, 40), SurroundColors = new[] { Color.FromArgb(0, 0, 0) } })
				g.FillPath(b, path);
		}
		Text(g, "FIN", CW / 2f, CH * 0.47f, 'd', 130f, Color.FromArgb(240, 236, 226));
		Text(g, "?", CW / 2f + 160, CH * 0.47f, 'i', 60f, Color.FromArgb(120, 118, 112));
	}

	// Cole's card, for Scene 6: written back to front, so it reads true from behind the screen.
	static void Mirrored(Graphics g)
	{
		using (var bmp = new Bitmap(CW, CH))
		{
			using (var h = Graphics.FromImage(bmp))
			{
				h.SmoothingMode = SmoothingMode.AntiAlias;
				h.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
				Backdrop(h, 12, 26);
				Border(h, 140);
				Text(h, "COLE —", CW / 2f, CH * 0.30f, 'd', 58f, Color.FromArgb(238, 234, 224));
				Text(h, "if you can read this,", CW / 2f, CH * 0.52f, 'i', 44f, Color.FromArgb(220, 216, 206));
				Text(h, "you're behind the screen. Wave.", CW / 2f, CH * 0.70f, 'i', 44f, Color.FromArgb(220, 216, 206));
			}
			bmp.RotateFlip(RotateFlipType.RotateNoneFlipX);
			g.DrawImageUnscaled(bmp, 0, 0);
		}
	}

	// ---- the drawn scenes -------------------------------------------------------------------------------------------------

	static void Sky(Graphics g, int top, int bottom)
	{
		using (var b = new LinearGradientBrush(new Rectangle(0, 0, CW, CH), Color.FromArgb(top, top, top), Color.FromArgb(bottom, bottom, bottom), 90f))
			g.FillRectangle(b, 0, 0, CW, CH);
	}

	static Brush Ink(int v) { return new SolidBrush(Color.FromArgb(v, v, v)); }

	/** A man in a fedora and long coat: feet at (x, y), h tall, facing +1 right or -1 left. */
	static void Man(Graphics g, float x, float y, float h, int face, int v, bool can)
	{
		var b = Ink(v);
		float s = h / 180f;
		g.FillEllipse(b, x - 11 * s, y - h + 18 * s, 22 * s, 24 * s);							// head
		g.FillRectangle(b, x - 20 * s, y - h + 18 * s, 40 * s, 4 * s);							// brim
		g.FillRectangle(b, x - 11 * s, y - h + 6 * s, 22 * s, 14 * s);							// crown
		g.FillPolygon(b, new[] { new PointF(x - 20 * s, y - h + 46 * s), new PointF(x + 20 * s, y - h + 46 * s),
			new PointF(x + 28 * s, y - 40 * s), new PointF(x - 28 * s, y - 40 * s) });				// coat
		g.FillPolygon(b, new[] { new PointF(x - 16 * s, y - 42 * s), new PointF(x - 4 * s, y - 42 * s), new PointF(x - 6 * s, y), new PointF(x - 16 * s, y) });
		g.FillPolygon(b, new[] { new PointF(x + 4 * s, y - 42 * s), new PointF(x + 16 * s, y - 42 * s), new PointF(x + 16 * s, y), new PointF(x + 6 * s, y) });
		if (can)
		{
			g.FillEllipse(b, x + face * 18 * s - 24 * s, y - h * 0.62f - 24 * s, 48 * s, 48 * s);
			g.FillEllipse(Ink(Math.Min(255, v + 60)), x + face * 18 * s - 6 * s, y - h * 0.62f - 6 * s, 12 * s, 12 * s);
		}
	}

	static void Paris(Graphics g)
	{
		Sky(g, 150, 60);
		var save = Rng; Rng = new Random(11);
		BlackSun(g, CW * 0.70f, CH * 0.30f, 48f, 52);
		Rng = save;
		// rooftops
		var roofs = new List<PointF> { new PointF(0, CH) };
		float x = 0;
		var r = new Random(3);
		while (x < CW)
		{
			float w = 40 + r.Next(70), hgt = CH * (0.55f + 0.18f * (float)r.NextDouble());
			roofs.Add(new PointF(x, hgt)); roofs.Add(new PointF(x + w * 0.2f, hgt - 18)); roofs.Add(new PointF(x + w * 0.8f, hgt - 18)); roofs.Add(new PointF(x + w, hgt));
			x += w;
		}
		roofs.Add(new PointF(CW, CH));
		g.FillPolygon(Ink(16), roofs.ToArray());
		// the tower
		float tx = CW * 0.28f, base_ = CH * 0.72f, top = CH * 0.06f;
		g.FillPolygon(Ink(12), new[] { new PointF(tx - 70, base_), new PointF(tx - 8, top + 40), new PointF(tx - 3, top), new PointF(tx + 3, top), new PointF(tx + 8, top + 40), new PointF(tx + 70, base_), new PointF(tx + 40, base_), new PointF(tx, base_ - 70), new PointF(tx - 40, base_) });
		using (var p = new Pen(Color.FromArgb(12, 12, 12), 5f)) g.DrawLine(p, tx - 40, base_ - 120, tx + 40, base_ - 120);
		// lit windows
		for (int i = 0; i < 40; i++) g.FillRectangle(Ink(150 + r.Next(80)), r.Next(CW), CH * 0.72f + r.Next((int)(CH * 0.25f)), 5, 7);
	}

	static void Blinds(Graphics g)
	{
		g.Clear(Color.FromArgb(20, 20, 20));
		// light through the blinds across a wall
		for (int i = 0; i < 9; i++)
		{
			float y = 20 + i * 48;
			using (var b = new LinearGradientBrush(new RectangleF(0, y, CW, 26), Color.FromArgb(205, 205, 200), Color.FromArgb(60, 60, 58), 0f))
				g.FillPolygon(b, new[] { new PointF(0, y), new PointF(CW, y + 90), new PointF(CW, y + 116), new PointF(0, y + 26) });
		}
		// her silhouette at the window: head, neck, shoulders, a cigarette's thread of smoke
		var ink = Ink(8);
		float cx = CW * 0.62f, cy = CH * 0.40f;
		g.FillEllipse(ink, cx - 46, cy - 60, 92, 112);
		g.FillEllipse(ink, cx - 70, cy - 82, 120, 90);			// the hair
		g.FillRectangle(ink, cx - 20, cy + 40, 40, 50);
		g.FillPolygon(ink, new[] { new PointF(cx - 150, CH), new PointF(cx - 120, cy + 90), new PointF(cx + 120, cy + 90), new PointF(cx + 150, CH) });
		using (var p = new Pen(Color.FromArgb(170, 170, 165), 2f))
		{
			var pts = new List<PointF>();
			for (int i = 0; i < 30; i++) pts.Add(new PointF(cx - 120 + (float)Math.Sin(i * 0.5) * 12, cy + 30 - i * 9));
			g.DrawCurve(p, pts.ToArray());
		}
	}

	static void Drive(Graphics g)
	{
		Sky(g, 18, 8);
		// the road to the vanishing point, the centre line
		float vx = CW * 0.5f, vy = CH * 0.42f;
		g.FillPolygon(Ink(28), new[] { new PointF(vx - 6, vy), new PointF(vx + 6, vy), new PointF(CW * 0.98f, CH), new PointF(CW * 0.02f, CH) });
		for (int i = 0; i < 7; i++)
		{
			float t0 = (float)Math.Pow(i / 7.0, 1.6), t1 = (float)Math.Pow((i + 0.5) / 7.0, 1.6);
			g.FillPolygon(Ink(200), new[] { new PointF(vx - 2 - 10 * t0, vy + (CH - vy) * t0), new PointF(vx + 2 + 10 * t0, vy + (CH - vy) * t0), new PointF(vx + 2 + 10 * t1, vy + (CH - vy) * t1), new PointF(vx - 2 - 10 * t1, vy + (CH - vy) * t1) });
		}
		// the car from behind, its tail lights, its headlights' wash ahead
		using (var path = new GraphicsPath())
		{
			path.AddEllipse(vx - 260, vy - 40, 520, 160);
			using (var b = new PathGradientBrush(path) { CenterColor = Color.FromArgb(180, 230, 230, 220), SurroundColors = new[] { Color.FromArgb(0, 0, 0, 0) } })
				g.FillPath(b, path);
		}
		float cx = vx, cy = CH * 0.80f;
		g.FillPolygon(Ink(6), new[] { new PointF(cx - 190, cy + 50), new PointF(cx - 180, cy - 10), new PointF(cx - 110, cy - 30), new PointF(cx - 80, cy - 90), new PointF(cx + 80, cy - 90), new PointF(cx + 110, cy - 30), new PointF(cx + 180, cy - 10), new PointF(cx + 190, cy + 50) });
		g.FillEllipse(Ink(240), cx - 170, cy - 2, 44, 18);
		g.FillEllipse(Ink(240), cx + 126, cy - 2, 44, 18);
		// trees either side
		var r = new Random(5);
		for (int i = 0; i < 14; i++)
		{
			float side = i % 2 == 0 ? -1 : 1, t = (float)Math.Pow((i / 2) / 7.0 + 0.05, 1.5);
			float tx = vx + side * (30 + 520 * t), ty = vy + (CH - vy) * t, th = 30 + 380 * t;
			g.FillPolygon(Ink(4), new[] { new PointF(tx, ty - th), new PointF(tx + th * 0.22f, ty), new PointF(tx - th * 0.22f, ty) });
		}
	}

	static void Roof(Graphics g)
	{
		Sky(g, 120, 40);
		var save = Rng; Rng = new Random(23);
		BlackSun(g, CW * 0.24f, CH * 0.28f, 40f, 44);
		Rng = save;
		// chimneys and the roof's edge
		g.FillPolygon(Ink(14), new[] { new PointF(0, CH * 0.70f), new PointF(CW, CH * 0.62f), new PointF(CW, CH), new PointF(0, CH) });
		g.FillRectangle(Ink(14), CW * 0.12f, CH * 0.52f, 40, CH * 0.2f);
		g.FillRectangle(Ink(14), CW * 0.84f, CH * 0.44f, 54, CH * 0.2f);
		Man(g, CW * 0.58f, CH * 0.66f, 230f, 1, 6, true);
	}

	static void Stair(Graphics g)
	{
		g.Clear(Color.FromArgb(30, 30, 30));
		// a shaft of light and a long stair
		g.FillPolygon(Ink(150), new[] { new PointF(CW * 0.30f, 0), new PointF(CW * 0.55f, 0), new PointF(CW * 0.95f, CH), new PointF(CW * 0.48f, CH) });
		for (int i = 0; i < 9; i++)
		{
			float y = CH * 0.50f + i * 26, x0 = CW * 0.10f + i * 70;
			g.FillRectangle(Ink(10), x0, y, CW, CH);
		}
		Man(g, CW * 0.32f, CH * 0.50f + 2, 200f, 1, 4, false);
		Man(g, CW * 0.62f, CH * 0.50f + 5 * 26 + 2, 200f, -1, 4, false);
		// their shadows thrown up the wall
		Man(g, CW * 0.80f, CH * 0.42f, 300f, -1, 90, false);
	}

	static void Eclipse(Graphics g)
	{
		g.Clear(Color.FromArgb(14, 14, 14));
		var save = Rng; Rng = new Random(31);
		BlackSun(g, CW * 0.66f, CH * 0.46f, 120f, 90);
		Rng = save;
		// a face in profile looking up into it
		var ink = Ink(2);
		float x = CW * 0.18f, y = CH;
		g.FillPolygon(ink, new[] { new PointF(x - 140, y), new PointF(x - 140, CH * 0.20f), new PointF(x + 40, CH * 0.12f), new PointF(x + 90, CH * 0.30f),
			new PointF(x + 120, CH * 0.40f), new PointF(x + 96, CH * 0.44f), new PointF(x + 104, CH * 0.52f), new PointF(x + 92, CH * 0.56f),
			new PointF(x + 98, CH * 0.62f), new PointF(x + 70, CH * 0.70f), new PointF(x + 60, CH * 0.82f), new PointF(x + 40, y) });
	}

	static void Projector(Graphics g)
	{
		g.Clear(Color.FromArgb(10, 10, 10));
		// the beam, widening to the right, with dust in it
		using (var b = new LinearGradientBrush(new RectangleF(CW * 0.28f, 0, CW * 0.72f, CH), Color.FromArgb(210, 210, 205), Color.FromArgb(40, 40, 40), 0f))
			g.FillPolygon(b, new[] { new PointF(CW * 0.30f, CH * 0.48f), new PointF(CW, CH * 0.02f), new PointF(CW, CH * 0.98f), new PointF(CW * 0.30f, CH * 0.56f) });
		var r = new Random(9);
		for (int i = 0; i < 160; i++)
		{
			float x = CW * (0.35f + 0.65f * (float)r.NextDouble()), spread = (x - CW * 0.30f) / (CW * 0.70f);
			float y = CH * 0.52f + (float)(r.NextDouble() - 0.5) * CH * 0.9f * spread;
			g.FillEllipse(Ink(230), x, y, 2.5f, 2.5f);
		}
		// the machine: body, lens, the two reels
		var ink = Ink(4);
		g.FillRectangle(ink, CW * 0.10f, CH * 0.44f, CW * 0.20f, CH * 0.22f);
		g.FillRectangle(ink, CW * 0.29f, CH * 0.47f, 30, 34);
		g.FillRectangle(ink, CW * 0.16f, CH * 0.66f, 26, CH * 0.34f);
		foreach (var c in new[] { new PointF(CW * 0.14f, CH * 0.26f), new PointF(CW * 0.25f, CH * 0.30f) })
		{
			g.FillEllipse(ink, c.X - 62, c.Y - 62, 124, 124);
			for (int k = 0; k < 3; k++)
			{
				double a = k * Math.PI * 2 / 3 + 0.4;
				g.FillEllipse(Ink(70), c.X + (float)Math.Cos(a) * 30 - 14, c.Y + (float)Math.Sin(a) * 30 - 14, 28, 28);
			}
		}
	}

	static void Vault(Graphics g)
	{
		Backdrop(g, 6, 34);
		float cx = CW / 2f, cy = CH * 0.52f, R = 190f;
		using (var p = new Pen(Color.FromArgb(150, 150, 146), 10f)) g.DrawEllipse(p, cx - R, cy - R, R * 2, R * 2);
		g.FillEllipse(Ink(40), cx - R + 14, cy - R + 14, (R - 14) * 2, (R - 14) * 2);
		for (int k = 0; k < 12; k++)
		{
			double a = k * Math.PI * 2 / 12;
			g.FillEllipse(Ink(170), cx + (float)Math.Cos(a) * (R - 32) - 9, cy + (float)Math.Sin(a) * (R - 32) - 9, 18, 18);
		}
		using (var p = new Pen(Color.FromArgb(200, 200, 196), 12f))
		{
			for (int k = 0; k < 3; k++)
			{
				double a = k * Math.PI / 3 + 0.3;
				g.DrawLine(p, cx - (float)Math.Cos(a) * 90, cy - (float)Math.Sin(a) * 90, cx + (float)Math.Cos(a) * 90, cy + (float)Math.Sin(a) * 90);
			}
		}
		g.FillEllipse(Ink(220), cx - 26, cy - 26, 52, 52);
		Text(g, "never to be seen by common eyes again", cx, CH * 0.06f + 14, 'i', 26f, Color.FromArgb(170, 166, 156));
	}

	static void Still(Graphics g, string path)
	{
		if (!File.Exists(path))
		{
			Inter(g, "« ... »", "(the reel is scratched here)");
			return;
		}
		using (var img = Image.FromFile(path))
		{
			// the middle band of the square picture, to the cell's shape
			float srcH = img.Width * (float)CH / CW;
			var src = new RectangleF(0, (img.Height - srcH) * 0.45f, img.Width, srcH);
			var attr = new ImageAttributes();
			float[][] m = {
				new float[] { 0.30f, 0.30f, 0.30f, 0, 0 },
				new float[] { 0.59f, 0.59f, 0.59f, 0, 0 },
				new float[] { 0.11f, 0.11f, 0.11f, 0, 0 },
				new float[] { 0, 0, 0, 1, 0 },
				new float[] { 0, 0, 0, 0, 1 } };
			attr.SetColorMatrix(new ColorMatrix(m));
			g.DrawImage(img, new Rectangle(0, 0, CW, CH), src.X, src.Y, src.Width, src.Height, GraphicsUnit.Pixel, attr);
		}
	}
}
