## AdaptiveBinarize

Adaptive Binarize for Vapoursynth, based on [OpenCV's Adaptive Thresholding](https://docs.opencv.org/5.x/d7/d4d/tutorial_py_thresholding.html).

### Usage
```python
abrz.AdaptiveBinarize(vnode clip, vnode clip2[, int c=3])
```
### Parameters:

- clip\
    A clip to process. It must be in YUV/GRAY 8-bit.

- clip2\
    Blured clip to calculate the thresholding.\
    Gauss has a cleaner result and Mean/BoxBlur retains more detail.
    
- c\
    Controls the threshold, read the OpenCV doc for more details.\
    Default: 3.

### Example
This code in Vapoursynth:
```python
import vsutil
import vapoursynth as vs
core = vs.core

src8 = ...
luma = vsutil.get_y(src8)
luma16 = vsutil.depth(luma, 16)
blur16 = luma16.std.Convolution([1]*11, mode='hv')
blur8 = depth(blur16, 8)
binarize = core.abrz.AdaptiveBinarize(src8, blur8, c=3)
```

Has the same result as this in OpenCV:
```python
binarize = cv.adaptiveThreshold(img, 255, cv.ADAPTIVE_THRESH_MEAN_C, cv.THRESH_BINARY_INV, 11, 3)
```