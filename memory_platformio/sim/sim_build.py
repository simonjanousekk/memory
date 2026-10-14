# PlatformIO pre-script for [env:native]: links SDL2 + libcurl, tells the sim
# where its fixtures live, and skips the Adafruit GFX display drivers (TFT /
# OLED) that need real hardware headers. The sim only uses GFXcanvas1.
import os

Import("env")

env.ParseConfig("sdl2-config --cflags --libs")
env.Append(LIBS=["curl"])
env.Append(CXXFLAGS=["-std=gnu++17"])
env.Append(CPPDEFINES=[("SIM_DIR", env.StringifyMacro(os.path.join(env.subst("$PROJECT_DIR"), "sim")))])

for pattern in ("*Adafruit_SPITFT.cpp", "*Adafruit_GrayOLED.cpp"):
    env.AddBuildMiddleware(lambda node: None, pattern)
