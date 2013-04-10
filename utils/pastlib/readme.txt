            -=[ Pixel Art Scaling Toolkit - v0.2 by WolfWings ]=-

This is a low-level graphics-scaling library intented primarilly for use with
low-resolution 'pixel art' such as that found in emulation. It will implement
most scaling methods with a relatively fixed time, though it may require more
work than some other choices to add to your software. On a Core 2 Duo it will
render HQ2x (one of the most demanding algo's currently) in around 21.5 clock
cycles per output pixel without any MMX/SSE code, while HQ4x costs half that,
approximately 10.75 clock cycles per output pixel. So quadruple the scale, at
only double the cost.

Please examine and understand the pastlib.h file, it is how you'd modify the
library to support other resolutions and/or pixel formats. And please do not
try to make the library accomidate dynamic horizontal resolutions, it's just
not possible without suffering a very measurable 20-25% speed loss on x86-64
or even worse on plain x86 with even fewer registers available.

Also, note that this code is developed under GCC 4.3, and takes advantage of
the profile optimized compiling technique. For example, the 'unguided' build
of the algo is almost a third slower than the profiled version. There's some
work to be done yet to integrate MMX code and the like, but for now enjoy.

And yes, I will be creating tables for HQ3x, 2xSai, Scale2x, etc, etc. While
most of these will likely be redundant, and perhaps inefficient, they'll all
be entirely ISC Licensed implementations at that point, allowing use in many
situations where their current licenses would prevent that.

- WolfWings, who is way too good at making full-justified text.
