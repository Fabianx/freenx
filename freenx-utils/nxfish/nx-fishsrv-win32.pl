#!/usr/bin/perl

# nx-fishrv.pl - Originally based on KDE sources.
#
#   A simple fileserver to allow remote browsing via nxfish://.
# 
# License: GPL - v2
#
# Made "secure" for use by NX by Fabian Franz <freenx@fabian-franz.de>.
# 
# Version: 0.0.3-nx-3
#
# SVN: $$ 

=pod
=cut

use Fcntl;
$|++;
use strict;
use POSIX qw(getcwd dup2 strftime);
use IO::Socket ();

my $substpath=$ARGV[0];
my $cookie=$ARGV[1];
my $port=$ARGV[2];
my $authenticated=0;

my $client_ich=undef;
my $client_och=undef;

if ($substpath eq '' or $substpath eq '.')
{
	$substpath='./';
}

if ($cookie eq '')
{
	$authenticated=1;
}

$| = 1;

if ($port ne '')
{
	Server();
}
else
{
	Run(*STDIN, *STDOUT);
}

sub Server()
{
  my $verbose=1;
  my $debug=0;
  $verbose = 1 if $debug && !$verbose;

  my $ah = IO::Socket::INET->new('LocalAddr' => "0.0.0.0",
				 'LocalPort' => $port,
				 'Reuse' => 1,
				 'Listen' => 10)
    || die "Failed to bind to local socket: $!";

  print "Entering main loop.\n" if $verbose;
  $SIG{'CHLD'} = 'IGNORE';
  my $num = 0;
  while (1) {
    my $ch = $ah->accept();
    if (!$ch) {
      print STDERR "Failed to accept: $!\n";
      next;
    }
    printf("Accepting client from %s, port %s.\n",
	   $ch->peerhost(), $ch->peerport()) if $verbose;
    ++$num;
    my $pid = eval { fork () };
    if ($@) {
      # fork not supported, we handle a single connection
      Run($ch, $ch);
    } elsif (!defined($pid)) {
      print STDERR "Failed to fork: $!\n";
    } elsif ($pid == 0) {
      # This is the child
      $ah->close();
      Run($ch, $ch);
      exit 0;
    } else {
      print "Parent: Forked child, closing socket.\n" if $verbose;
      $ch->close();
    }
  }
}

sub Run()
{
$SIG{'CHLD'} = 'IGNORE';
$client_ich = shift;
$client_och = shift;

myprint("FISH:\n");

MAIN: while (readline($client_ich)) {
    chomp;
    chomp;

    write_file("debug.log", "$_\n"); # debug
    
    /^#PASS\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $usercookie    = $1;
	
	if ($usercookie eq $cookie or $authenticated eq 1)
	{
		$authenticated=1;
		#myprint("### 200\n");
	}
	else
	{
		myprint("### 500 Wrong password supplied.\n");
	}
        next; # or should we bail out here?
    };
    
    if ($authenticated eq 0)
    {
    	myprint("### 500 Not authenticated please use #PASS command.\n");
	next;
    }

    /\#FISH/ && do {
	myprint("### 200\n");
	next;
    };

    next if !length($_) || substr( $_, 0, 1 ) ne '#';
    s/^#//;
    /^VER / && do {
        myprint("VER 0.0.3 copy lscount lslinks lsmime stat\n### 200\n");
        next;
    };
    /^PWD$/ && do {
        myprint("/", "\n### 200\n");
        next;
    };
    /^SYMLINK\s+((?:\\.|[^\\])*?)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $ofn = unquote($1);
        my $fn  = unquote($2);
	#myprint( symlink( $ofn, $fn ) ? "### 200\n" : "### 500 $!\n" );
	myprint( "### 500 Symlink not supported on Windows systems.\n" );
        next;
    };
    /^COPY\s+((?:\\.|[^\\])*?)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $ofn    = unquote($1);
        my $fn     = unquote($2);
        my ($size) = ( stat($ofn) )[7];
        my $read   = 1;
        if ( -l $ofn ) {
            my $dest = readlink($ofn);
            unlink($fn);
            symlink( $dest, $fn ) || ( $read = 0 );
        }
        else {
            sysopen( FH, $ofn, O_RDONLY ) || do { myprint("### 500 $!\n"); next; };
            sysopen( OFH, $fn, O_WRONLY | O_CREAT | O_TRUNC )
              || do { close(FH); myprint("### 500 $!\n"); next; };
            local $/ = undef;
            my $buffer = '';
            while ( $size > 16384
                && ( $read = sysread( FH, $buffer, 16384 ) ) > 0 )
            {
                $size -= $read;
                if ( syswrite( OFH, $buffer, $read ) != $read ) {
                    close(FH);
                    close(OFH);
                    myprint("### 500 $!\n");
                    next MAIN;
                }
            }
            while ( $size > 0 && ( $read = sysread( FH, $buffer, $size ) ) > 0 )
            {
                $size -= $read;
                if ( syswrite( OFH, $buffer, $read ) != $read ) {
                    close(FH);
                    close(OFH);
                    myprint("### 500 $!\n");
                    next MAIN;
                }
            }
            close(FH);
            close(OFH);
        }
        if ( $read > 0 ) {
            myprint("### 200\n");
        }
        else {
            myprint("### 500 $!\n");
        }
        next;
    };
    /^LINK\s+((?:\\.|[^\\])*?)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $ofn = unquote($1);
        my $fn  = unquote($2);
        myprint( link( $ofn, $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^RENAME\s+((?:\\.|[^\\])*?)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $ofn = unquote($1);
        my $fn  = unquote($2);
        myprint( rename( $ofn, $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^CHGRP\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($2);
        myprint( chown( -1, int($1), $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^CHOWN\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($2);
        myprint( chown( int($1), -1, $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^CHMOD\s+([0-7]+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($2);
        myprint( chmod( oct($1), $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^DELE\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($1);
        myprint( unlink($fn) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^RMD\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $dn = unquote($1);
        myprint( rmdir($dn) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^MKD\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $dn = unquote($1);
        if ( mkdir( $dn, 0777 ) ) {
            myprint("### 200\n");
        }
        else {
            my $err = $!;
            myprint( chdir($dn) ? "### 501 $err\n" : "### 500 $err\n" );
        }
        next;
    };
    #/^CWD\s+((?:\\.|[^\\])*?)\s*$/ && do {
    #    my $dn = unquote($1);
    #    myprint( chdir($dn) ? "### 200\n" : "### 500 $!\n" );
    #    next;
    #};
    /^LIST\s+((?:\\.|[^\\])*?)\s*$/ && do {
        list( $1, 1 );
        next;
    };
    /^STAT\s+((?:\\.|[^\\])*?)\s*$/ && do {
        list( $1, 0 );
        next;
    };
    /^WRITE\s+(\d+)\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        write_loop( $2, $3, O_WRONLY | O_CREAT, $1 );
        next;
    };
    /^APPEND\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        write_loop( $1, $2, O_WRONLY | O_APPEND );
        next;
    };
    /^STOR\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        write_loop( $1, $2, O_WRONLY | O_CREAT | O_TRUNC );
        next;
    };
    /^RETR\s+((?:\\.|[^\\])*?)\s*$/ && do {
        read_loop($1);
        next;
    };
    /^READ\s+(\d+)\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        read_loop( $3, $2, $1 );
        next;
    };
}
exit(0);
}

sub write_file
{
	my $f = shift;
	my $data = shift;
        open F, ">> $f" or die "Can't open $f : $!";
        syswrite(F, $data, length($data));
        close F;
}


sub list {
    my $dn = unquote( $_[0] );
    my @entries;
    if ( !-e $dn ) {
        myprint("### 404 File does not exist\n");
        return;
    }
    elsif ( $_[1] && -d _ ) {
        opendir( DIR, $dn ) || do { myprint("### 500 $!\n"); return; };
        @entries = readdir(DIR);
        closedir(DIR);
    }
    else {
        ( $dn, @entries ) = $dn =~ m{(.*)/(.*)};
        $dn = '/' if ( !length($dn) );
    }
    myprint(scalar(@entries), "\n### 100\n");
    my $cwd = getcwd();
    chdir($dn) || do { myprint("### 500 $!\n"); return; };
    foreach (@entries) {
        my $link = readlink;
        my ( $mode, $uid, $gid, $size, $mtime ) = (lstat)[ 2, 4, 5, 7, 9 ];
        myprint(filetype( $mode, $link, $uid, $gid ));
        myprint("S$size\n");
        myprint(strftime( "D%Y %m %d %H %M %S\n", localtime($mtime) ));
        myprint(":$_\n");
        myprint("L$link\n") if (defined $link && ($_[0] ne "/"));
        myprint(mimetype($_));
        myprint("\n");
    }
    chdir($cwd);
    myprint("### 200\n");
}

sub read_loop {
    my $fn = unquote( $_[0] );
    my ($size) = ( $_[1] ? int( $_[1] ) : ( stat($fn) )[7] );
    my $error = '';
    myprint("### 501 Is directory\n") and return if -d $fn;
    sysopen( FH, $fn, O_RDONLY ) || ( $error = $! );
    if ( $_[2] ) {
        sysseek( FH, int( $_[2] ), 0 ) || do { close(FH); $error ||= $!; };
    }
    myprint("### 500 $error\n") and return if $error;
    if ( @_ < 2 ) {
        myprint("$size\n");
    }
    myprint("### 100\n");
    my $buffer = '';
    my $read   = 1;
    while ( $size > 16384 && ( $read = sysread( FH, $buffer, 16384 ) ) > 0 ) {
        $size -= $read;
        myprint($buffer);
    }
    while ( $size > 0 && ( $read = sysread( FH, $buffer, $size ) ) > 0 ) {
        $size -= $read;
        myprint($buffer);
    }
    while ( $size > 0 ) {
        myprint(' ');
        $size--;
    }
    $error ||= $! if $read <= 0;
    close(FH);
    if ( !$error ) {
        myprint("### 200\n");
    }
    else {
        myprint("### 500 $error\n");
    }
}

sub write_loop {
    my $size  = int( $_[0] );
    my $fn    = unquote( $_[1] );
    my $error = '';
    sysopen( FH, $fn, $_[2] ) || do { myprint("### 400 $!\n"); return; };
    eval { flock( FH, 2 ); };
    if ( $_[3] ) {
        sysseek( FH, int( $_[3] ), 0 )
          || do { close(FH); myprint("### 400 $!\n"); return; };
    }
    readline($client_ich);
    myprint("### 100\n");
    my $buffer = '';
    my $read   = 1;
    while ( $size > 16384 && ( $read = read( $client_ich, $buffer, 16384 ) ) > 0 ) {
        $size -= $read;
        $error ||= $! if ( syswrite( FH, $buffer, $read ) != $read );
    }
    while ( $size > 0 && ( $read = read( $client_ich, $buffer, $size ) ) > 0 ) {
        $size -= $read;
        $error ||= $! if ( syswrite( FH, $buffer, $read ) != $read );
    }
    close(FH);
    if ( !$error ) {
        myprint("### 200\n");
    }
    else {
        myprint("### 500 $error\n");
    }
}

# FIXME: Make it secure

sub unquote { $_ = shift; s/\\(.)/$1/g; s/\.\./\//g; s/^/$substpath/g; return $_; }
#sub unquote { $_ = shift; s/\\(.)/$1/g; return $_; }

sub myprint {
	my $foo = shift;
	syswrite($client_och, $foo, length($foo));
}


sub filetype {
    my ( $mode, $link, $uid, $gid ) = @_;
    my $result = 'P';
    while (1) {
        -f _           && do { $result .= '-'; last; };
        -d _           && do { $result .= 'd'; last; };
        defined($link) && do { $result .= 'l'; last; };
        -c _           && do { $result .= 'c'; last; };
        -b _           && do { $result .= 'b'; last; };
        -S _           && do { $result .= 's'; last; };
        -p _           && do { $result .= 'p'; last; };
        $result .= '?';
        last;
    }
    $result .= ( $mode & 0400 ? 'r' : '-' );
    $result .= ( $mode & 0200 ? 'w' : '-' );
    $result .=
      ( $mode & 0100
        ? ( $mode & 04000 ? 's' : 'x' )
        : ( $mode & 04000 ? 'S' : '-' ) );
    $result .= ( $mode & 0040 ? 'r' : '-' );
    $result .= ( $mode & 0020 ? 'w' : '-' );
    $result .=
      ( $mode & 0010
        ? ( $mode & 02000 ? 's' : 'x' )
        : ( $mode & 02000 ? 'S' : '-' ) );
    $result .= ( $mode & 0004 ? 'r' : '-' );
    $result .= ( $mode & 0002 ? 'w' : '-' );
    $result .=
      ( $mode & 0001
        ? ( $mode & 01000 ? 't' : 'x' )
        : ( $mode & 01000 ? 'T' : '-' ) );
    $result .= ' ';
    $result .= '1000'; #( getpwuid($uid) || $uid );
    $result .= '.';
    $result .= '1000'; #( getgrgid($gid) || $gid );
    $result .= "\n";
    return $result;
}

sub mimetype {
    my $fn = shift;
    return "Minode/directory\n" if -d $fn;
    pipe( IN, OUT );
    my $pid = fork();
    return '' if ( !defined $pid );
    if ($pid) {
        close(OUT);
        my $type = <IN>;
        close(IN);
        chomp $type;
        chomp $type;
        $type =~ s/[,; ].*//;
        return '' if ( $type !~ m/\// );
        return "M$type\n";
    }
    close(IN);
#    sysopen( NULL, '/dev/null', O_RDWR );
#    dup2( fileno(NULL), fileno(STDIN) );
#    dup2( fileno(OUT),  fileno(STDOUT) );
#    dup2( fileno(NULL), fileno(STDERR) );
    exec( 'C:\Programme\GnuWin32\bin\file.exe', '-i', '-b', $fn );
    exit(0);
}
