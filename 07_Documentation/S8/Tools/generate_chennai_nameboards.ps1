# Original complete-phrase artwork; uses installed Nirmala UI only while rendering.
# No font, character atlas, or third-party image is redistributed.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$output = Join-Path $projectRoot '02_Art/Stations/ChennaiNameboards01'
New-Item -ItemType Directory -Force -Path $output | Out-Null
$rows = @(
 @('Chennai Egmore','சென்னை எழும்பூர்','चेन्नै एग्मोर','MS'),
 @('Chennai Chetpet','சேத்துப்பட்டு','चेन्नै चेटपेट','MSC'),
 @('Nungambakkam','நுங்கம்பாக்கம்','नुंगम्बाक्कम','NBK'),
 @('Kodambakkam','கோடம்பாக்கம்','कोडम्बाक्कम','MKK'),
 @('Mambalam','மாம்பலம்','मांबलम','MBM'),
 @('Saidapet','சைதாப்பேட்டை','सैदापेट','SP'),
 @('Guindy','கிண்டி','गिंडी','GDY'),
 @('St. Thomas Mount','பரங்கிமலை','सेंट थॉमस माउंट','STM'),
 @('Palavanthangal','பழவந்தாங்கல்','पलवन्तांगल','PZA'),
 @('Meenambakkam','மீனம்பாக்கம்','मीनम्बाक्कम','MN'),
 @('Tirusulam','திருசூலம்','तिरुसूलम','TLM'),
 @('Pallavaram','பல்லாவரம்','पल्लावरम','PV'),
 @('Chrompet','குரோம்பேட்டை','क्रोमपेट','CMP'),
 @('Tambaram Sanatorium','தாம்பரம் சானடோரியம்','ताम्बरम सैनिटोरियम','TBMS'),
 @('Tambaram','தாம்பரம்','ताम्बरम','TBM')
)
$fontFamily = [System.Drawing.FontFamily]::new('Nirmala UI')
if ($fontFamily.Name -ne 'Nirmala UI') { throw 'Nirmala UI not available: refuse fallback boxes' }
$records = @()
foreach ($i in 0..14) {
 $row=$rows[$i]
 $bitmap=[System.Drawing.Bitmap]::new(2048,512)
 $graphics=[System.Drawing.Graphics]::FromImage($bitmap)
 $graphics.Clear([System.Drawing.Color]::FromArgb(245,195,25))
 $graphics.TextRenderingHint=[System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
 $graphics.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
 $pen=[System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(40,34,21),8)
 $graphics.DrawRectangle($pen,12,12,2024,488)
 $format=[System.Drawing.StringFormat]::new()
 $format.Alignment=[System.Drawing.StringAlignment]::Center
 $format.LineAlignment=[System.Drawing.StringAlignment]::Center
 $format.FormatFlags=[System.Drawing.StringFormatFlags]::NoWrap
 $lines=@($row[1],$row[2],($row[0].ToUpperInvariant()+'  |  '+$row[3]))
 foreach ($lineIndex in 0..2) {
  $size=118
  do {
   $font=[System.Drawing.Font]::new($fontFamily,$size,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
   $measured=$graphics.MeasureString($lines[$lineIndex],$font)
   if($measured.Width -gt 1930) {$font.Dispose();$size-=2}
  } while($measured.Width -gt 1930)
  $rect=[System.Drawing.RectangleF]::new(40,(24+154*$lineIndex),1968,154)
  $graphics.DrawString($lines[$lineIndex],$font,[System.Drawing.Brushes]::Black,$rect,$format)
  $font.Dispose()
 }
 # Small painted mounting bolts, not glyphs, add a grounded hardware detail.
 foreach($x in @(30,2008)){foreach($y in @(30,472)){$graphics.FillEllipse([System.Drawing.Brushes]::DarkSlateGray,$x,$y,10,10)}}
 $name='T_Nameboard_{0:D2}.png' -f $i
 $path=Join-Path $output $name
 $bitmap.Save($path,[System.Drawing.Imaging.ImageFormat]::Png)
 $graphics.Dispose();$bitmap.Dispose();$pen.Dispose();$format.Dispose()
 $records+=@{index=$i;english=$row[0];tamil=$row[1];hindi=$row[2];code=$row[3];file=$name;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$manifest=@{version=1;dimensions=@(2048,512);renderer='Windows GDI+ shaped DrawString, installed Nirmala UI Bold';fontRedistributed=$false;individualGlyphAtlas=$false;licence='Original project phrase artwork; font rights: Microsoft font redistribution FAQ, graphic files in games/apps';licenceSource='https://learn.microsoft.com/en-us/typography/fonts/font-faq';stationNames='Tamil from retained OSM railway station nodes; duplicate Tambaram removed at Sanatorium. Hindi transliterations except retained Mambalam/Guindy/Pallavaram/Tambaram tags; not exact surveyed board copies.';osmAttribution='© OpenStreetMap contributors, ODbL 1.0 https://www.openstreetmap.org/copyright';boards=$records}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding utf8
$fontFamily.Dispose()
Write-Output "Rendered 15 complete trilingual nameboards in $output"
