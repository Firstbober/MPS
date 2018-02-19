#include "app.h"
#include "lib/opengl.h"
#include "error.h"
#include "transform.h"
#include "lib/sdl_mixer.h"
#include "texload.h"
#include "lib/sdl_ttf.h"
#include "defines.h"
#include <sstream> 

#include "lib/sdl_image.h"

#include "render/buffer.h"

using namespace std;

void App::init() {
 
    window_w=1280;
    window_h=720;
    App::proportions = 16.0/9.0;

	SDL_version ver;
	SDL_GetVersion(&ver);
	if(ver.minor==0){
		if(ver.patch<7){
			stringstream ss;
			ss<<"Required SDL version: 2.0.7. You have "<<(int)ver.major<<"."<<(int)ver.minor<<"."<<(int)ver.patch<<endl<<"The program may not work correctly.";
			error(ss.str());
		}
	}

    if(SDL_Init(SDL_INIT_VIDEO)!=0) error("Cannot init SDL");
	
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_ES
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
	
    #ifndef FASTSTART
	SDL_Surface * splash = surfLoad("splash.png");
	if(!splash){
        error("Cannot load splash screen!");
        exit(0);
    }
    loadingwindow = SDL_CreateShapedWindow("Loading...",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED, splash->w, splash->h, SDL_WINDOW_HIDDEN|SDL_WINDOW_SKIP_TASKBAR);
    SDL_Surface * loadingwindowsurface = SDL_GetWindowSurface(loadingwindow);
    
    SDL_WindowShapeMode shapemode;
    shapemode.mode = ShapeModeDefault;
    SDL_SetWindowShape(loadingwindow, splash, &shapemode);
    SDL_BlitSurface(splash,NULL,loadingwindowsurface,NULL);
    SDL_FreeSurface(splash);
    SDL_SetWindowOpacity(loadingwindow,0.0);
    SDL_UpdateWindowSurface(loadingwindow);
    SDL_ShowWindow(loadingwindow);

	int pX,pY;
	SDL_GetWindowPosition(loadingwindow,&pX,&pY);
    for(int i=0;i<=26;i+=1){
        SDL_Event evt;
        while(SDL_PollEvent(&evt));
        SDL_SetWindowOpacity(loadingwindow,i/25.0);
		SDL_SetWindowPosition(loadingwindow,pX,pY+(25-i)/3);
        SDL_UpdateWindowSurface(loadingwindow);
        SDL_Delay(20);
    }
    SDL_Delay(1500);
    #endif

    if(TTF_Init()==-1) error("Cannot init SDL_ttf");
    std::string fontfile = std::string(std::string(LOC_ROOT)+std::string("Lato-Light.ttf"));
    font10 = TTF_OpenFont(fontfile.c_str(),10);
    font16 = TTF_OpenFont(fontfile.c_str(),16);
    font24 = TTF_OpenFont(fontfile.c_str(),24);
    font32 = TTF_OpenFont(fontfile.c_str(),32);
    font64 = TTF_OpenFont(fontfile.c_str(),64);
    font128 = TTF_OpenFont(fontfile.c_str(),128);
    if(!font10||!font16||!font32||!font64||!font128) {
        stringstream ss;
        ss<<"Cannot load the fonts! SDL_TTF error: "<<endl<<TTF_GetError();
        error(ss.str());
        exit(0);
    }

#ifdef __ANDROID__
    window = SDL_CreateWindow("Moving Picture Studio",0,0,window_w,window_h,SDL_WINDOW_OPENGL);glDisable(GL_DITHER);
#else
    window = SDL_CreateWindow("Moving Picture Studio",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,window_w,window_h,SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN|SDL_WINDOW_FULLSCREEN_DESKTOP);
    if(window==NULL) error("Cannot create main OpenGL window");
#endif
    context = SDL_GL_CreateContext(window);
    #ifdef GLEW_STATIC
        glewExperimental=true;
        glewInit();
    #endif // GLEW_STATIC
    if(context==NULL) error("Cannot create OpenGL context");

    SDL_GL_SetSwapInterval(vsync);
    SDL_GetWindowSize(window,&window_w,&window_h);
    window_w_prev=window_w;window_h_prev=window_h;
    keystate = SDL_GetKeyboardState(NULL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
	//glEnable(GL_MULTISAMPLE);

    static const GLfloat square_vert_data[] = {0,0, 0,1, 1,1, 0,0, 1,0, 1,1};
	square_vert = new RBuffer;
	square_vert->bind().setData(sizeof(square_vert_data), square_vert_data, GL_STATIC_DRAW);

    static const GLfloat square_uv_data[] = { 0,0, 0,1, 1,1, 0,0, 1,0, 1,1};
	square_uv = new RBuffer;
	square_uv->bind().setData(sizeof(square_uv_data), square_uv_data, GL_STATIC_DRAW);

    for(uint i=0;i<16;i++){
        glEnableVertexAttribArray(i);//this should be all, but..
        glVertexAttribPointer(i,2,GL_FLOAT,GL_FALSE,0,(void*)0);//some shitty drivers crash so i must fill with something
    }
	

    shMan["default"].load("shaders/default.vsh","shaders/default.fsh");
    shMan["default2d"].load("shaders/default2d.vsh","shaders/default2d.fsh");
    shMan["texture2d"].load("shaders/texture2d.vsh","shaders/texture2d.fsh").createUniform("COLOR");
    shMan["color2d"].load("shaders/color2d.vsh","shaders/color2d.fsh").createUniform("COLOR");

    App::updateProportions();

    Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,1024);

    fps_text.create();
    updateDebugger();

    App::runApplication();
}

int App::getWindowWidth() {
    return window_w;
}

int App::getWindowHeight() {
    return window_h;
}

float App::getProportions() {
    return proportions;
}

void App::setProportions(float prop){
    proportions = prop;
}

bool App::eventHandle(SDL_Event * evt) {
    bool p = SDL_PollEvent(evt);
    if(p) {
        switch(evt->type) {
			case SDL_QUIT: {
				pushEvent(GlobalEvent::Exit);
				break;
			}
			case SDL_KEYDOWN: {
				switch(evt->key.keysym.sym) {
					case SDLK_AC_BACK: {
						pushEvent(GlobalEvent::Exit);
						break;
					}
					case SDLK_F4: {
						setFullscreen(!isFullscreen());
						break;
					}
					case SDLK_F3:{
						debugger=!debugger;
						updateDebugger();
						break;
					}
					case SDLK_F5:{
						vsync=!vsync;
						SDL_GL_SetSwapInterval(vsync);
						break;
					}
				}
				break;
			}
			case SDL_WINDOWEVENT:{
				switch(evt->window.event){
					case SDL_WINDOWEVENT_FOCUS_GAINED:{
						if(vsync){
							SDL_GL_SetSwapInterval(1);
						}
						hidden=false;
						break;
					}
					case SDL_WINDOWEVENT_FOCUS_LOST:{
						if(vsync){
							SDL_GL_SetSwapInterval(0);
						}
						hidden=true;
						break;
					}
				}
				break;
			}
        }
    }
    return p;
}


void App::updateAll() {
    App::updateWindow();
}

void App::updateProportions(){
    if(window_w>window_h*proportions) { //width-wide
        areawidth=window_h*proportions;
        areaheight=window_h;
    } else { //height-wide
        areawidth=window_w;
        areaheight=window_w/proportions;
    }
    areawidth*=areamultipler;
    areaheight*=areamultipler;
    if(areawidth<10)areawidth=10;
    if(areaheight<10)areaheight=10;
    window_w_prev=window_w;
    window_h_prev=window_h;

    pushEvent(GlobalEvent::WindowSizeChange);
}

int App::getAreaWidth(){
    return areawidth;
}

int App::getAreaHeight(){
    return areaheight;
}

void App::setAreaMultipler(float n){
    areamultipler=n;
}

void App::setAutoProportions(bool n){
    autoproportions=n;
}
bool App::getAutoProportions(){
    return autoproportions;
}

bool App::isHidden(){
	return hidden;
}

void App::updateWindow() {
	if(hidden){
		SDL_Delay(150);
	}
	else{
		#ifdef USE_FRAMEBUFFER
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		#endif
		glViewport(0, 0, window_w, window_h);
		if(debugger){//FPS count
			Shader & texture2d = shMan["texture2d"];
			texture2d.select();
			projection=glm::ortho(0.0f,(float)window_w,(float)window_h,0.0f);
			texture2d.setP(&projection);

			fps_text.select();
			
			square_vert->bind().attrib(0,2,GL_FLOAT);
			square_uv->bind().attrib(1,2,GL_FLOAT);

			xReset(&model);
			xTranslate(&model, 1, 1);
			xScale(&model,fps_text.getWidth(),fps_text.getHeight());
			texture2d.setUniform("COLOR", 0,0,0,1).setM(&model);
			glDrawArrays(GL_TRIANGLES, 0, 2*3);

			xReset(&model);
			xScale(&model,fps_text.getWidth(),fps_text.getHeight());
			texture2d.setUniform("COLOR", 1,1,1,1).setM(&model);
			glDrawArrays(GL_TRIANGLES, 0, 2*3);
		}
		
		glFinish();
		SDL_GL_SwapWindow(window);
	}
    keystate = SDL_GetKeyboardState(NULL);

    SDL_GetWindowSize(window,&window_w,&window_h);
    if(window_w_prev!=window_w||window_h_prev!=window_h) {
        if(autoproportions){
            setProportions(getWindowWidth()/(float)getWindowHeight());
        }
        App::updateProportions();
    }

    fps_frames++;
    if(fps_last+1000<SDL_GetTicks()) {
        fps_last=SDL_GetTicks();
        fps=fps_frames-1;
        fps_frames=0;
        if(debugger&&!hidden){
            updateDebugger();
        }
    }
}

void App::blitFramebuffer(Framebuffer * buffer){
    #ifdef USE_FRAMEBUFFER
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_w, window_h);
    Shader & default2d = shMan["default2d"];
    default2d.select();
    glBindTexture(GL_TEXTURE_2D,buffer->getTexture());
    glBindBuffer(GL_ARRAY_BUFFER, square_vert);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, square_uv);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)0);

    xReset(&model);
    if(window_w>window_h*proportions) { //width-wide
        xTranslate(&model,0.5-(window_h*proportions)/(float)window_w/2.0, 0.0);
        xScale(&model, (window_h*proportions)/(float)window_w, 1.0f);
    } else { //height-wide
        xTranslate(&model,0.0, 0.5-(window_w/proportions)/(float)window_h/2.0);
        xScale(&model, 1.0f, (window_w/proportions)/(float)window_h);
    }
    default2d.setM(&model);
    projection=glm::ortho(0.0f,1.0f,1.0f,0.0f);
    default2d.setP(&projection);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    #endif
}

void App::updateDebugger(){
    stringstream ss;
    ss<<fps<<" FPS";
    texRenderText(fps_text,font16,ss.str().c_str(),SDL_Color{255,255,255},false);
}

bool App::onEvent() {
    if(events.size()>0) {
        return true;
    } else {
        return false;
    }
}

GlobalEvent App::getEvent() {
    if(events.size()>0) {
        GlobalEvent event = events.at(0);
        events.erase(events.begin());
        return event;
    } else {
        return GlobalEvent::Nothing;
    }
}

void App::pushEvent(GlobalEvent event) {
    events.push_back(event);
}

bool App::isFullscreen() {
    return fullscreen;
}

void App::setFullscreen(bool state) {
    fullscreen=state;
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &current);
    if(fullscreen==false){
        SDL_SetWindowFullscreen(window,0);
        SDL_SetWindowSize(window,1280, 720);
        SDL_SetWindowBordered(window,SDL_TRUE);
        int x,y;
        SDL_GetWindowPosition(window,&x,&y);
        SDL_SetWindowPosition(window,x+40,y+40);
        updateProportions();
    }
    else{
        SDL_SetWindowBordered(window,SDL_FALSE);
        SDL_SetWindowSize(window,current.w,current.h);
        SDL_SetWindowFullscreen(window,SDL_WINDOW_FULLSCREEN);
        updateProportions();
    }
}

bool App::isPressed(int scancode){
    return keystate[scancode];
}

