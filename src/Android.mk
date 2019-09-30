LOCAL_PATH := $(call my-dir)
VBSP_DIR = ../../vbsp-gles/
ENGINE_DIR =../../nenuzhno-engine_iter1/src/
include $(CLEAR_VARS)

LOCAL_MODULE    := nenuzhno-engine
LOCAL_CFLAGS    := -Wall -D_VBSP -std=c++11
LOCAL_C_INCLUDES := $(LOCAL_PATH)/. $(LOCAL_PATH)/../../libs/glm/glm $(LOCAL_PATH)/$(ENGINE_DIR)
LOCAL_SRC_FILES := $(ENGINE_DIR)android_backend.cpp $(ENGINE_DIR)system/FileSystem.cpp $(ENGINE_DIR)system/config.cpp $(ENGINE_DIR)log.cpp $(ENGINE_DIR)game/IGame.cpp $(ENGINE_DIR)cull/frustum.cpp $(ENGINE_DIR)cull/BoundingBox.cpp \
	$(ENGINE_DIR)graphics/gl_utils.cpp $(ENGINE_DIR)graphics/glsl_prog.cpp $(ENGINE_DIR)graphics/texture.cpp $(ENGINE_DIR)graphics/fbo.cpp $(ENGINE_DIR)graphics/vbo.cpp \
	$(ENGINE_DIR)renderer/font.cpp $(ENGINE_DIR)renderer/camera.cpp $(ENGINE_DIR)renderer/Model.cpp \
	$(ENGINE_DIR)resource/ResourceManager.cpp $(ENGINE_DIR)resource/vtf_loader.cpp $(ENGINE_DIR)resource/dds_loader.cpp $(ENGINE_DIR)resource/nmf_loader.cpp \
	vbsp.cpp input.cpp vmt_loader.cpp mdl_loader.cpp bsp_loader.cpp parser.cpp \
	renderer/bsp_renderer.cpp renderer/mdl_renderer.cpp renderer/progs_manager.cpp renderer/material.cpp renderer/render_list.cpp bsp_cull.cpp \
	bsp_physics.cpp physics.cpp
LOCAL_LDLIBS := -llog -lGLESv2 -lm -lEGL
LOCAL_STATIC_LIBRARIES =

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../libs/bullet3-2.86.1/src
LOCAL_LDLIBS += -lBullet
LOCAL_LDFLAGS += -L$(LOCAL_PATH)/../../libs/bullet3-2.86.1/lib

include $(BUILD_SHARED_LIBRARY)
