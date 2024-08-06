# 当前文件所在目录
LOCAL_PATH := $(call my-dir)

#---------------------------------------

# 清除 LOCAL_xxx 变量
include $(CLEAR_VARS)

# 当前模块名
LOCAL_MODULE := $(notdir $(LOCAL_PATH))

# 模块对外头文件（只能是目录）
# 加载至CFLAGS中提供给其他组件使用；打包进SDK产物中；
LOCAL_TUYA_SDK_INC := $(LOCAL_PATH)/include
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/bk_core
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/ezlopi_core
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/ezlopi_cloud
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/ezlopi_services
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/ezlopi_util
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/ezlopi_user_config
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/ezlopi_hal
LOCAL_TUYA_SDK_INC += $(LOCAL_PATH)/ezlopi_sensor_devices


# 模块对外CFLAGS：其他组件编译时可感知到
LOCAL_TUYA_SDK_CFLAGS := -DUSER_SW_VER=\"$(APP_VER)\" -DAPP_BIN_NAME=\"$(APP_NAME)\"

# 模块源代码
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH)/src -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/bk_core -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/ezlopi_core -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/ezlopi_cloud -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/ezlopi_services -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/ezlopi_util -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/ezlopi_user_config -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/ezlopi_hal -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/ezlopi_sensor_devices -name "*.c" -o -name "*.cpp" -o -name "*.cc")

# 模块内部CFLAGS：仅供本组件使用
LOCAL_CFLAGS += -DDEBUG_WOLFSSL
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/portable/Keil/ARM968es
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/func/wolfssl
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/func/wolfssl/wolfssl/wolfcrypt
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/app/config
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/os/FreeRTOSv9.0.0/FreeRTOS/Source/include
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/os/include
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/func/user_driver/
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/common/
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/driver/common/
LOCAL_CFLAGS += -I$(TUYA_SDK_PATH)/software/TuyaOS/vendor/bk7231n/bk7231n_os/beken378/driver/include/
# 全局变量赋值
TUYA_SDK_INC += $(LOCAL_TUYA_SDK_INC)  # 此行勿修改
TUYA_SDK_CFLAGS += $(LOCAL_TUYA_SDK_CFLAGS)  # 此行勿修改

# 生成静态库
include $(BUILD_STATIC_LIBRARY)

# 生成动态库
include $(BUILD_SHARED_LIBRARY)

# 导出编译详情
include $(OUT_COMPILE_INFO)

#---------------------------------------

