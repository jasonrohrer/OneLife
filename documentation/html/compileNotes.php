<?php include( "header.php" ); ?>

<font size=6>Compiling from source</font><br>
<table border=0 width="100%" cellpadding=5><tr><td bgcolor="#222222">
Make sure you have the following packages installed:
<ul>
  <li>git (source control management)
  <li>g++ (compiler)
  <li>imagemagick (image conversion tools)
  <li>xclip (access to X Windows clipboard)
</ul>
Make sure you have the <b>dev</b> package of the following libraries installed:
<ul>
<li>libsdl1.2
<li>libgl     
<li>libglu     
</ul>
On Ubuntu, for example, the package names are:
<ul>
<li>git
<li>g++
<li>imagemagick
<li>xclip
<li>libsdl1.2-dev  
<li>libglu1-mesa-dev  
<li>libgl1-mesa-dev
</ul>
If you want to do this from the command line, you would run:
<pre><code>    sudo apt-get install git g++ imagemagick xclip libsdl1.2-dev libglu1-mesa-dev libgl1-mesa-dev</code></pre>

<br>
<br>
After installing the required packages on your system, download the UnixSource bundle and extract it:

<pre><code>    tar xzf OneLife_Live4_UnixSource.tar.gz</code></pre>

The included "pullAndBuildLatest" script should pull from git and build the game for you automatically.  You would run it like this:

<pre><code>    cd OneLife_Live4_UnixSource
    ./pullAndBuildLatest
</code></pre>

<br>
     
After that, you can run the game like this:

<pre><code>    ./OneLifeApp
</code></pre>

     

</td></tr></table>
<br>
<br>

<?php include( "footer.php" ); ?>
