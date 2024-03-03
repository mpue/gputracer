
# GPUTracer

GPUTracer is in fact an offline Version of [Shadertoy](https://www.shadertoy.com/) 

Although it is in a very early stage, it already can run Shaders from the online site.

![Screenshot](https://raw.githubusercontent.com/mpue/gputracer/main/doc/screen1.png)


View and edit multiple shaders at once.

![Screenshot](https://raw.githubusercontent.com/mpue/gputracer/main/doc/screen1.png)

Currently you can edit shaders, preview and render the images to disk as well.

The shaders from Shadertoy, are not 100% compatible, but will be in the future. Currently you must add the following code to the main shader:

    void main() {
    

    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dims = imageSize(outputImage); // fetch image dimensions
        
    vec4 outColor;

    mainImage(outColor,gl_GlobalInvocationID.xy);

    imageStore(outputImage, pixelCoords, outColor);
    }    


# Contribute

Every contributon is welcome at any time.
