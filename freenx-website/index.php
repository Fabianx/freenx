<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
	<head>
		<title>
			FreeNX - the free NX
		</title>
		<link rel="stylesheet" type="text/css" href="style.css">
		<meta http-equiv="content-type" content="text/html; charset=ISO-8859-1">
	</head>
	<body>
		<div class="header">
			<h1>
				FreeNX
			</h1>
			<p>
				Free Software (GPL) Implementation of the NX Server
			</p>
		</div>
		<div style="position:relative;">
			<div class="navigation">
				<div class="box">
					<h2>
						FreeNX
					</h2>
					<a href="http://openfacts2.berlios.de/wikien/index.php/BerliosProject:FreeNX_-_FAQ">
						Frequently Asked Questions (FAQ)
					</a>
					<a href="info.php">
						Information
					</a>
					<a href="download.php">
						Download
					</a>
				</div>
				<div class="box">
					<h2>
						Development
					</h2>
					<a href="http://developer.berlios.de/projects/freenx/">
                                                Project Page
                                        </a>
					<a href="https://mail.kde.org/mailman/listinfo/freenx-knx/">
						Mailing List
					</a>
					<a href="http://developer.berlios.de/bugs/?group_id=2978">
						File a Bug
					</a>
					<a href="http://developer.berlios.de/feature/?group_id=2978">
						Request a Feature
					</a>
				</div>
				<div class="box">
					<h2>
						NX
					</h2>
					<a href="http://nomachine.com/">
						NoMachine
					</a>
					<a href="clients.php">
						Clients
					</a>
				</div>
			</div>
			<div class="content">
<?

	if (file_exists(substr($SCRIPT_URL,1,-4).".inc"))
		include substr($SCRIPT_URL,1,-4).".inc";
	else
		if (file_exists($page.".inc"))
			include $page.".inc";
		else
			include "home.inc";

?>
				<div>
					<a href="http://developer.berlios.de">
						<img src="http://developer.berlios.de/bslogo.php?group_id=2978" width="124" height="32" border="0" alt="BerliOS Logo" title="this project is hosted on BerliOS">
					</a>
				</div>
			</div>
		</div>
	</body>
</html>
