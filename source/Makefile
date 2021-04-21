all:
	cd bmp;make
	cd control;make
	cd lguicore;make
	cd gdi;make
	cd hdc;make
	cd ial;make
	cd ime;make
	cp bmp/libbmp.so ../lib/libbmp.so
	cp control/libcontrol.so ../lib/libcontrol.so
	cp lguicore/liblguicore.so ../lib/liblguicore.so
	cp gdi/libgdi.so ../lib/libgdi.so
	cp hdc/libhdc.so ../lib/libhdc.so
	cp ial/libial.so ../lib/libial.so
	cp ime/libime.so ../lib/libime.so

	$(CC) -g -o lgui lgui.c -L../lib/ -lbmp -lcontrol -llguicore -lgdi -lhdc -lial -lime -lpthread

	cp lgui ../bin/lgui
	$(CC) -g -o notepad notepad.c -L../lib/ -lbmp -lcontrol -llguicore -lgdi -lhdc -lial -lime -lpthread
	cp notepad ../bin/notepad

	$(CC) -g -o draw draw.c -L../lib/ -lbmp -lcontrol -llguicore -lgdi -lhdc -lial -lime -lpthread
	cp draw ../bin/draw

	$(CC) -g -o message message.c -L../lib/ -lbmp -lcontrol -llguicore -lgdi -lhdc -lial -lime -lpthread
	cp message ../bin/message

	$(CC) -g -o computerart computerart.c -L../lib/ -lbmp -lcontrol -llguicore -lgdi -lhdc -lial -lime -lpthread
	cp computerart ../bin/computerart

clean:
	rm -rf ../lib/*.so
	cd bmp;make clean
	cd control;make clean
	cd lguicore;make clean
	cd gdi;make clean
	cd hdc;make clean
	cd ial;make clean
	cd ime;make clean
	
	rm ../bin/lgui
	rm notepad 
	rm ../bin/notepad
	rm draw 
	rm ../bin/draw
	rm message
	rm ../bin/message
	rm computerart
	rm ../bin/computerart







