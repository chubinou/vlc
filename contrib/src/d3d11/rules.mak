# generate Direct3D11 temporary include

ifdef HAVE_CROSS_COMPILE
IDL_INCLUDES = -I/usr/include/wine/windows/ -I/usr/include/wine/wine/windows/
else
#ugly way to get the default location of standard idl files
IDL_INCLUDES = -I/`echo $(MSYSTEM) | tr A-Z a-z`/$(BUILD)/include
endif

DST_D3D11_H = $(PREFIX)/include/d3d11.h
DST_D3D11_H = $(PREFIX)/include/d3d11.h
DST_DXGIDEBUG_H = $(PREFIX)/include/dxgidebug.h
DST_DXGITYPE_H = $(PREFIX)/include/dxgitype.h
DST_DXGIFORMAT_H = $(PREFIX)/include/dxgiformat.h
DST_DXGICOMMON_H = $(PREFIX)/include/dxgicommon.h
DST_DXGI_IDL = $(PREFIX)/include/dxgi.idl
DST_DXGI_H = $(PREFIX)/include/dxgi.h
DST_DXGI12_H = $(PREFIX)/include/dxgi1_2.h
DST_DXGI13_H = $(PREFIX)/include/dxgi1_3.h
DST_DXGI14_H = $(PREFIX)/include/dxgi1_4.h
DST_DXGI15_H = $(PREFIX)/include/dxgi1_5.h
DST_DXGI16_H = $(PREFIX)/include/dxgi1_6.h
DST_D3D11_1_H = $(PREFIX)/include/d3d11_1.h
DST_D3D11_2_H = $(PREFIX)/include/d3d11_2.h
DST_D3D11_3_H = $(PREFIX)/include/d3d11_3.h

ifdef HAVE_WIN32
PKGS += d3d11
endif

#.sum-d3d11: $(TARBALLS)/d3d11.idl  $(TARBALLS)/d3d11_1.h $(TARBALLS)/d3d11_2.h $(TARBALLS)/d3d11_3.h $(TARBALLS)/dxgidebug.idl $(TARBALLS)/dxgi1_2.idl $(TARBALLS)/dxgitype.idl $(TARBALLS)/dxgiformat.idl $(TARBALLS)/dxgi.idl  $(TARBALLS)/dxgicommon.idl

d3d11:
	mkdir -p $@
	cp  $(SRC)/d3d11/d3d11.idl $@ && cd $@ && patch -fp1 < ../$(SRC)/d3d11/processor_format.patch

$(DST_D3D11_H): d3d11
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $</d3d11.idl

$(DST_D3D11_1_H): d3d11 $(SRC)/d3d11/d3d11_1.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(SRC)/d3d11/d3d11_1.h $@

$(DST_D3D11_2_H): d3d11 $(SRC)/d3d11/d3d11_2.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(SRC)/d3d11/d3d11_2.h $@

$(DST_D3D11_3_H): d3d11 $(SRC)/d3d11/d3d11_3.h
	mkdir -p -- "$(PREFIX)/include/"
	cp $(SRC)/d3d11/d3d11_3.h $@

$(DST_DXGIDEBUG_H): $(SRC)/d3d11/dxgidebug.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGITYPE_H): $(SRC)/d3d11/dxgitype.idl $(DST_DXGICOMMON_H) $(DST_DXGIFORMAT_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGIFORMAT_H): $(SRC)/d3d11/dxgiformat.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGICOMMON_H): $(SRC)/d3d11/dxgicommon.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI_H): $(SRC)/d3d11/dxgi.idl
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI12_H): $(SRC)/d3d11/dxgi1_2.idl $(DST_DXGI_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -I$(PREFIX)/include -I$(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI13_H): $(SRC)/d3d11/dxgi1_3.idl $(DST_DXGI12_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI14_H): $(SRC)/d3d11/dxgi1_4.idl $(DST_DXGI13_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI15_H): $(SRC)/d3d11/dxgi1_5.idl $(DST_DXGI14_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

$(DST_DXGI16_H): $(SRC)/d3d11/dxgi1_6.idl $(DST_DXGI15_H)
	mkdir -p -- "$(PREFIX)/include/"
	$(WIDL) -DBOOL=WINBOOL -Idxgi12 -I$(PREFIX)/include $(IDL_INCLUDES) -h -o $@ $<

.d3d11: $(DST_D3D11_H) $(DST_D3D11_1_H) $(DST_D3D11_2_H)  $(DST_D3D11_3_H) \
	$(DST_DXGI_H)  $(DST_DXGI12_H) $(DST_DXGI13_H) $(DST_DXGI14_H) $(DST_DXGI15_H) $(DST_DXGI16_H) \
	$(DST_DXGITYPE_H) $(DST_DXGIFORMAT_H) $(DST_DXGICOMMON_H) \
	$(DST_DXGIDEBUG_H)
	touch $@
