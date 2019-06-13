Image Processing algorithm that check if all 4 LEDs are on from test image. Supposed that LEDs have approximately circle form, the same size and located approx on one disatnce from image center

**third-party required:**

```
OpenCV 
```


**Command line:**

```
LEDsFinder.exe "path to image file"
```

**Example of Usage:**
```
LEDsFinder.exe good_4.png
```

**Example of Algorithm result:**


![alt text](https://github.com/cppjocker/LEDsFinder/tree/master/img/good_4_recognized.png)

**Description of algorithm:**


Firs, I split image on 4 parts. For each part, I search max intenensity, take 0.8 threshold and perform binarization by such threshold. 

Than, I use binary image to find contours of objects. For each countour I calculate numerical features - how much figure is close to circle, location relatively to image center, supposed radius and so on.

Also I perform simple filtrarion - contours too small, too differ from circle form.

Than I collect all contours from 4 quarters of original image and collect them in clusters. The idea is that to collect in cluster contours from 4 quarters with the same distance to center - there must be myrror symmetry from all 4 LEDs with relation to center.

So, if it is impossible to construct cluster (there is a quarter where there is no spot on expected distance) - I make decision that LED is not found. In the case when for some quarter there are several contours satisfying criteria - I choose contour with best circularity.

When cluster is found - I check that each spot posses enough good circularity and in such case I make a decision than all 4 LEDs are found.                                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                  
