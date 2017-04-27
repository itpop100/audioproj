---------------------------------------------------------------
               Audio Streamer Project - Readme
---------------------------------------------------------------
#BCIT CST NoName Group

#Fred Yang, John Agapeyev, Maitiu Morton, Isaac Morneau

This readme outlines the steps on how to run the application.

---------------------------------------------------------------
                 Audio Streamer - Server Side
---------------------------------------------------------------
1. The project uses libZPlay multimedia library. libZplay is one 
   for playing mp3, mp2, mp1, ogg, flac, ac3, aac, oga, wav and 
   pcm files and streams. 
2. In order for program to work, the directory for the libzplay.lib
   must be placed in your project folder. 
3. To link the libzplay.lib, right click project Porperties, then 
   go to configuration properties>linker>input, add "libzplay.lib"
   in the field "additional dependencies". 
4. libzplay.dll must also be placed into your windows/system32 and
   windows/syswow64.
5. Open the Release folder and run "AudioStreamer.exe".

---------------------------------------------------------------
                 Audio Player - Client Side
---------------------------------------------------------------
1. Open the Release folder and run "AudioPlayer.exe".
2. Once the client window pops up, enter the hostname or IP, and 
   port # (7000 by default), then click on the menu File>Connect.
3. When the play list of songs shows up, you can choose one, then click
   on the "Play" button on the right side to play it.
4. Press "Pause" to pause the music, press "Rewind"/"Forward" to rewind/
   forward the music.
5. Press "Stop" to stop the music.
6. There are 4 options below the play list: 
   Download - choose to download a music from the server
   Upload - choose to upload a music to the server
   Multicast - choose to join the multicast group to listen to the music
   Microphone Chat - choose to make a 2-way microphone chatting session
