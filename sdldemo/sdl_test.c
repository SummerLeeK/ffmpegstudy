//
//  sdl_test.c
//  demo
//
//  Created by Apple on 2019/4/4.
//  Copyright © 2019年 Apple. All rights reserved.
//

#include <stdio.h>
#include <SDL2/SDL.h>

/**
*   加载一张图片 返回一个texture
*/
SDL_Texture * loadBmp(char *filename,SDL_Renderer *render){
    SDL_Surface *bitmap=NULL;
    SDL_Texture * texture;
    bitmap=SDL_LoadBMP(filename);
    if (!bitmap) {
        printf("load bmp failed %s\n",SDL_GetError());
        
        return NULL;
    }
    
    texture=SDL_CreateTextureFromSurface(render, bitmap);
    
    SDL_FreeSurface(bitmap);
    
    return texture;
    
    
}


void drawTexture(int x,int y,SDL_Texture *texture,SDL_Renderer *render){
    SDL_Rect pos;
    pos.x=x;
    pos.y=y;
    
    //计算texture的宽高 计算出位置并b赋值给pos
    SDL_QueryTexture(texture, NULL, NULL, &pos.w, &pos.h);
    
    SDL_RenderCopy(render, texture, NULL, &pos);
}


int maintest(int argc,char *argv[]){
    SDL_Window *window;
    SDL_Renderer *renderer;
    
    SDL_Texture *background;
    SDL_Texture *foreground;

    //初始化
    SDL_Init(SDL_INIT_VIDEO);
    
    //SDL_WINDOW_BORDERLESS 无边框
    //创建窗口
    window= SDL_CreateWindow("hahaha", 0, 0, 384, 216, SDL_WINDOW_SHOWN);
    
    if (!window) {
        printf("SDL_CreateWindow failed\n");
    }
    //创建渲染器
    renderer=SDL_CreateRenderer(window, -1, 0);
    
    if (!renderer) {
        printf("SDL_GetRenderer failed\n");
        goto _Destory_Window;
    }
    
    SDL_RenderClear(renderer);
    
    
    //加载一张bmp格式的图片 其他文件格式不支持
    background=loadBmp("/Users/apple/Desktop/MV/大鱼/周深 - 大鱼_48.bmp", renderer);
    
    foreground=loadBmp("/Users/apple/Desktop/MV/大鱼/周深 - 大鱼_72.bmp", renderer);
    
  
    drawTexture(0,0, background, renderer);
    
    drawTexture(192,0, background, renderer);
    drawTexture(0,108, background, renderer);
    drawTexture(192,108, background, renderer);
    
    drawTexture(96, 54, foreground, renderer);

    //将数据绘制到背景上
    SDL_RenderPresent(renderer);
    
    
    SDL_Delay(30000);
_Destory_Texture:
    SDL_DestroyTexture(background);
    SDL_DestroyTexture(foreground);
_Destory_Render:
    SDL_DestroyRenderer(renderer);
_Destory_Window:
    SDL_DestroyWindow(&window);

    SDL_Quit();
    return 0;
}



