package com.diaochan.audiodemo;

public class WangyiPlayer {
    static {
        System.loadLibrary("wangyiplayer");
    }

    /**
     * audio.mp3 -> output.pcm
     */
    public native void sound(String input,String output);
}
