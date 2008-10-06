CC = gcc
CXX = g++

CFLAGS = -m32 -g -Wall -pipe
#CFLAGS += -O2
#CFLAGS += -DNDEBUG -O6 -ffast-math -fomit-frame-pointer -march=pentium3 -mtune=pentium4

CFLAGS += -Isrc -Isrc/include -Isrc/win95 -Isrc/avp -Isrc/avp/win95 -Isrc/avp/support -Isrc/avp/win95/frontend -Isrc/avp/win95/gadgets
CFLAGS += $(shell sdl-config --cflags) $(shell openal-config --cflags)
CXXFLAGS = $(CFLAGS)

LDLIBS = -m32 $(shell sdl-config --libs) $(shell openal-config --libs)

ROOT = main.c files.c winapi.c stubs.c version.c mathline.c opengl.c oglfunc.c openal.c cdplayer.c menus.c net.c frustum.c kshape.c map.c maths.c md5.c mem3dc.c mem3dcpp.cpp module.c morph.c object.c shpanim.c sphere.c tables.c vdb.c
AVP = ai_sight.c avpview.c bh_agun.c bh_ais.c bh_alien.c bh_binsw.c bh_cable.c bh_corpse.c bh_deathvol.c bh_debri.c bh_dummy.c bh_fan.c bh_far.c bh_fhug.c bh_gener.c bh_ldoor.c bh_lift.c bh_light.c bh_lnksw.c bh_ltfx.c bh_marin.c bh_mission.c bh_near.c bh_pargen.c bh_plachier.c bh_plift.c bh_pred.c bh_queen.c bh_rubberduck.c bh_selfdest.c bh_snds.c bh_spcl.c bh_swdor.c bh_track.c bh_types.c bh_videoscreen.c bh_waypt.c bh_weap.c bh_xeno.c bonusabilities.c cconvars.cpp cdtrackselection.cpp cheatmodes.c comp_map.c comp_shp.c consolelog.cpp davehook.cpp deaths.c decal.c detaillevels.c dynamics.c dynblock.c equipmnt.c extents.c game.c game_statistics.c gamecmds.cpp gamevars.cpp hmodel.c hud.c inventry.c language.c lighting.c load_shp.c los.c mempool.c messagehistory.c missions.cpp movement.c paintball.c particle.c pfarlocs.c pheromon.c player.c pmove.c psnd.c psndproj.c pvisible.c savegame.c scream.cpp secstats.c sfx.c stratdef.c targeting.c track.c triggers.c weapons.c
SHAPES = cube.c
SUPPORT = consbind.cpp consbtch.cpp coordstr.cpp daemon.cpp indexfnt.cpp r2base.cpp r2pos666.cpp reflist.cpp refobj.cpp rentrntq.cpp scstring.cpp strtab.cpp strutil.c trig666.cpp wrapstr.cpp
AVPWIN95 = avpchunk.cpp cheat.c chtcodes.cpp d3d_hud.cpp ddplat.cpp endianio.c ffread.cpp ffstdio.cpp gammacontrol.cpp hierplace.cpp iofocus.cpp jsndsup.cpp kzsort.c langplat.c modcmds.cpp npcsetup.cpp objsetup.cpp pathchnk.cpp platsup.c pldghost.c pldnet.c progress_bar.cpp projload.cpp scrshot.cpp strachnk.cpp system.c usr_io.c vision.c
FRONTEND = avp_envinfo.c avp_intro.cpp avp_menudata.c avp_menus.c avp_mp_config.cpp avp_userprofile.cpp
GADGETS = ahudgadg.cpp conscmnd.cpp conssym.cpp consvar.cpp gadget.cpp hudgadg.cpp rootgadg.cpp t_ingadg.cpp teletype.cpp textexp.cpp textin.cpp trepgadg.cpp
WIN95 = animchnk.cpp animobs.cpp awtexld.cpp awbmpld.cpp awiffld.cpp awpnmld.cpp bmpnames.cpp chnkload.cpp chnktexi.cpp chnktype.cpp chunk.cpp chunkpal.cpp db.c debuglog.cpp dummyobjectchunk.cpp enumchnk.cpp enumsch.cpp envchunk.cpp fail.c fragchnk.cpp gsprchnk.cpp hierchnk.cpp huffman.cpp iff.cpp iff_ilbm.cpp ilbm_ext.cpp io.c list_tem.cpp ltchunk.cpp media.cpp mishchnk.cpp obchunk.cpp oechunk.cpp our_mem.c plat_shp.c plspecfn.c shpchunk.cpp sndchunk.cpp sprchunk.cpp string.cpp texio.c toolchnk.cpp txioctrl.cpp wpchunk.cpp zsp.cpp

# the following should really be autogenerated...

SRCNAMES =	$(addprefix $(2)/,$(1))
OBJNAMES =	$(addprefix $(2)/,$(addsuffix .o,$(basename $(1))))

ROOTSRC =	$(call SRCNAMES,$(ROOT),src)
ROOTOBJ =	$(call OBJNAMES,$(ROOT),src)
AVPSRC	=	$(call SRCNAMES,$(AVP),src/avp)
AVPOBJ	=	$(call OBJNAMES,$(AVP),src/avp)
SHAPESSRC =	$(call SRCNAMES,$(SHAPES),src/avp/shapes)
SHAPESOBJ =	$(call OBJNAMES,$(SHAPES),src/avp/shapes)
SUPPORTSRC =	$(call SRCNAMES,$(SUPPORT),src/avp/support)
SUPPORTOBJ =	$(call OBJNAMES,$(SUPPORT),src/avp/support)
AVPWIN95SRC =	$(call SRCNAMES,$(AVPWIN95),src/avp/win95)
AVPWIN95OBJ =	$(call OBJNAMES,$(AVPWIN95),src/avp/win95)
FRONTENDSRC =	$(call SRCNAMES,$(FRONTEND),src/avp/win95/frontend)
FRONTENDOBJ =	$(call OBJNAMES,$(FRONTEND),src/avp/win95/frontend)
GADGETSSRC =	$(call SRCNAMES,$(GADGETS),src/avp/win95/gadgets)
GADGETSOBJ =	$(call OBJNAMES,$(GADGETS),src/avp/win95/gadgets)
WIN95SRC =	$(call SRCNAMES,$(WIN95),src/win95)
WIN95OBJ =	$(call OBJNAMES,$(WIN95),src/win95)

SRC = $(ROOTSRC) $(AVPSRC) $(SHAPESSRC) $(SUPPORTSRC) $(AVPWIN95SRC) $(FRONTENDSRC) $(GADGETSSRC) $(WIN95SRC)
OBJ = $(ROOTOBJ) $(AVPOBJ) $(SHAPESOBJ) $(SUPPORTOBJ) $(AVPWIN95OBJ) $(FRONTENDOBJ) $(GADGETSOBJ) $(WIN95OBJ)

all: avp

avp: $(OBJ)
	$(CXX) -o avp $(OBJ) $(LDLIBS)

clean:
	-rm -rf $(OBJ) avp

distclean: clean
	-rm -rf `find . \( -not -type d \) -and \
		\( -name '*~' -or -name '.#*' \) -type f -print`
