package com.diaochan.audiodemo;

import android.os.Bundle;
import android.os.Environment;
import android.view.View;

import androidx.appcompat.app.AppCompatActivity;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

    }


    public void play(View view) {
        WangyiPlayer wangyiPlayer = new WangyiPlayer();
        String input = new File(Environment.getExternalStorageDirectory(), "audio.mp3").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(), "output.pcm").getAbsolutePath();
        wangyiPlayer.sound(input, output);
    }
}
