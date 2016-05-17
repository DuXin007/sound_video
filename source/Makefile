src := #
src += main.c
src += startPage/startPage.c
src += sound/sound_ctrol.c
src += sound/play_sound.c
src += ./video/video_ctrol.c
src += ./video/play_video.c
src += oss.c
src += rgb_to_bmp.c
src += v4l2.c
src += yuyv_rgb_jpeg.c
src += fun_base.c



#export PKG_CONFIG_PATH=/opt/gtkdfb/lib/pkgconfig
PREFIX = /opt/gtkdfb
LDFLAGS=-L${PREFIX}/lib -Wl,-rpath,${PREFIX}/lib  

cc = arm-linux-gcc
cflags = -Wall -g `pkg-config --cflags --libs`
option = -ljpeg -llcd -L./
#option =  -I./include 
#option =  -I./include 
target = ./demo
target_dir = ./
#target2_dir = ../../execute/multimedia/

obj = $(src:%.c=%.o)
$(target):$(obj)
	$(cc) $^ -o $@ $(cflags) $(LDFLAGS) $(option)  -lpthread  
#	mkdir $(target2_dir)
#	cp $(target_dir)/image $(target_dir)/video $(target_dir)/picture $(target_dir)/wav $(target) $(target2_dir) -fr
%.o:%.c
	$(cc) -c $< -o $@ $(cflags) 
.phony:clean
clean:
	rm *.o	$(target) $(target2_dir) -fr
