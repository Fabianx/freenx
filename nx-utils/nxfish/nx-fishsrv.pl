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

my $substpath=$ARGV[0];
my $cookie=$ARGV[1];
my $authenticated=0;

if ($substpath eq '' or $substpath eq '.')
{
	$substpath='./';
}

if ($cookie eq '')
{
	$authenticated=1;
}

use strict;
use POSIX qw(getcwd dup2 strftime);
$SIG{'CHLD'} = 'IGNORE';
$| = 1;
MAIN: while (<STDIN>) {
    chomp;
    chomp;

    #print STDERR "$_\n"; # debug
    
    /^#PASS\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $usercookie    = $1;
	
	if ($usercookie eq $cookie or $authenticated eq 1)
	{
		$authenticated=1;
		#print "### 200\n";
	}
	else
	{
		print "### 500 Wrong password supplied.\n";
	}
        next; # or should we bail out here?
    };
    
    if ($authenticated eq 0)
    {
    	print "### 500 Not authenticated please use #PASS command.\n";
	next;
    }

    /\#FISH/ && do {
	print "### 200\n";
	next;
    };

    next if !length($_) || substr( $_, 0, 1 ) ne '#';
    s/^#//;
    /^VER / && do {
        print "VER 0.0.3 copy lscount lslinks lsmime stat\n### 200\n";
        next;
    };
    /^PWD$/ && do {
        print "/", "\n### 200\n";
        next;
    };
    /^SYMLINK\s+((?:\\.|[^\\])*?)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $ofn = unquote($1);
        my $fn  = unquote($2);
        print( symlink( $ofn, $fn ) ? "### 200\n" : "### 500 $!\n" );
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
            sysopen( FH, $ofn, O_RDONLY ) || do { print "### 500 $!\n"; next; };
            sysopen( OFH, $fn, O_WRONLY | O_CREAT | O_TRUNC )
              || do { close(FH); print "### 500 $!\n"; next; };
            local $/ = undef;
            my $buffer = '';
            while ( $size > 16384
                && ( $read = sysread( FH, $buffer, 16384 ) ) > 0 )
            {
                $size -= $read;
                if ( syswrite( OFH, $buffer, $read ) != $read ) {
                    close(FH);
                    close(OFH);
                    print "### 500 $!\n";
                    next MAIN;
                }
            }
            while ( $size > 0 && ( $read = sysread( FH, $buffer, $size ) ) > 0 )
            {
                $size -= $read;
                if ( syswrite( OFH, $buffer, $read ) != $read ) {
                    close(FH);
                    close(OFH);
                    print "### 500 $!\n";
                    next MAIN;
                }
            }
            close(FH);
            close(OFH);
        }
        if ( $read > 0 ) {
            print "### 200\n";
        }
        else {
            print "### 500 $!\n";
        }
        next;
    };
    /^LINK\s+((?:\\.|[^\\])*?)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $ofn = unquote($1);
        my $fn  = unquote($2);
        print( link( $ofn, $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^RENAME\s+((?:\\.|[^\\])*?)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $ofn = unquote($1);
        my $fn  = unquote($2);
        print( rename( $ofn, $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^CHGRP\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($2);
        print( chown( -1, int($1), $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^CHOWN\s+(\d+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($2);
        print( chown( int($1), -1, $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^CHMOD\s+([0-7]+)\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($2);
        print( chmod( oct($1), $fn ) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^DELE\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $fn = unquote($1);
        print( unlink($fn) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^RMD\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $dn = unquote($1);
        print( rmdir($dn) ? "### 200\n" : "### 500 $!\n" );
        next;
    };
    /^MKD\s+((?:\\.|[^\\])*?)\s*$/ && do {
        my $dn = unquote($1);
        if ( mkdir( $dn, 0777 ) ) {
            print "### 200\n";
        }
        else {
            my $err = $!;
            print( chdir($dn) ? "### 501 $err\n" : "### 500 $err\n" );
        }
        next;
    };
    #/^CWD\s+((?:\\.|[^\\])*?)\s*$/ && do {
    #    my $dn = unquote($1);
    #    print( chdir($dn) ? "### 200\n" : "### 500 $!\n" );
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

sub list {
    my $dn = unquote( $_[0] );
    my @entries;
    if ( !-e $dn ) {
        print "### 404 File does not exist\n";
        return;
    }
    elsif ( $_[1] && -d _ ) {
        opendir( DIR, $dn ) || do { print "### 500 $!\n"; return; };
        @entries = readdir(DIR);
        closedir(DIR);
    }
    else {
        ( $dn, @entries ) = $dn =~ m{(.*)/(.*)};
        $dn = '/' if ( !length($dn) );
    }
    print scalar(@entries), "\n### 100\n";
    my $cwd = getcwd();
    chdir($dn) || do { print "### 500 $!\n"; return; };
    foreach (@entries) {
        my $link = readlink;
        my ( $mode, $uid, $gid, $size, $mtime ) = (lstat)[ 2, 4, 5, 7, 9 ];
        print filetype( $mode, $link, $uid, $gid );
        print "S$size\n";
        print strftime( "D%Y %m %d %H %M %S\n", localtime($mtime) );
        print ":$_\n";
        print "L$link\n" if (defined $link && ($_[0] ne "/"));
        print mimetype($_);
        print "\n";
    }
    chdir($cwd);
    print "### 200\n";
}

sub read_loop {
    my $fn = unquote( $_[0] );
    my ($size) = ( $_[1] ? int( $_[1] ) : ( stat($fn) )[7] );
    my $error = '';
    print "### 501 Is directory\n" and return if -d $fn;
    sysopen( FH, $fn, O_RDONLY ) || ( $error = $! );
    if ( $_[2] ) {
        sysseek( FH, int( $_[2] ), 0 ) || do { close(FH); $error ||= $!; };
    }
    print "### 500 $error\n" and return if $error;
    if ( @_ < 2 ) {
        print "$size\n";
    }
    print "### 100\n";
    my $buffer = '';
    my $read   = 1;
    while ( $size > 16384 && ( $read = sysread( FH, $buffer, 16384 ) ) > 0 ) {
        $size -= $read;
        print $buffer;
    }
    while ( $size > 0 && ( $read = sysread( FH, $buffer, $size ) ) > 0 ) {
        $size -= $read;
        print $buffer;
    }
    while ( $size > 0 ) {
        print ' ';
        $size--;
    }
    $error ||= $! if $read <= 0;
    close(FH);
    if ( !$error ) {
        print "### 200\n";
    }
    else {
        print "### 500 $error\n";
    }
}

sub write_loop {
    my $size  = int( $_[0] );
    my $fn    = unquote( $_[1] );
    my $error = '';
    sysopen( FH, $fn, $_[2] ) || do { print "### 400 $!\n"; return; };
    eval { flock( FH, 2 ); };
    if ( $_[3] ) {
        sysseek( FH, int( $_[3] ), 0 )
          || do { close(FH); print "### 400 $!\n"; return; };
    }
    <STDIN>;
    print "### 100\n";
    my $buffer = '';
    my $read   = 1;
    while ( $size > 16384 && ( $read = read( STDIN, $buffer, 16384 ) ) > 0 ) {
        $size -= $read;
        $error ||= $! if ( syswrite( FH, $buffer, $read ) != $read );
    }
    while ( $size > 0 && ( $read = read( STDIN, $buffer, $size ) ) > 0 ) {
        $size -= $read;
        $error ||= $! if ( syswrite( FH, $buffer, $read ) != $read );
    }
    close(FH);
    if ( !$error ) {
        print "### 200\n";
    }
    else {
        print "### 500 $error\n";
    }
}

# FIXME: Make it secure

sub unquote { $_ = shift; s/\\(.)/$1/g; s/\.\./\//g; s/^/$substpath/g; return $_; }
#sub unquote { $_ = shift; s/\\(.)/$1/g; return $_; }

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
    $result .= ( getpwuid($uid) || $uid );
    $result .= '.';
    $result .= ( getgrgid($gid) || $gid );
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
    sysopen( NULL, '/dev/null', O_RDWR );
    dup2( fileno(NULL), fileno(STDIN) );
    dup2( fileno(OUT),  fileno(STDOUT) );
    dup2( fileno(NULL), fileno(STDERR) );
    exec( '/usr/bin/file', '-i', '-b', '-L', $fn );
    exit(0);
}
