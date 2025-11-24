Sensors
======================================================

One the radiance along a ray into the camera has been calculated, the radiance now needs to be converted to a pixel intensity such that it can be displayed.  We'll first outline the full process for doing this, and then outline how this can be simplified in situations where full radiometic accuracy is not required (such as creating a simple animation)

Radiance to Electrical Signal
-----------------------------

Recall the spectral radiance (that is, the radiance for a given wavelength :math:`\lambda`), has units of :math:`\frac{W}{m^2 sr}`.  Therefore, knowing the spectral radiance incident on a given pixel, :math:`L_p(\lambda)`, we can calculate the spectral power received by that pixel, :math:`\Phi_p(\lambda)` (units of :math:`W`), by simplying multiplying the radiance by the collection area of our optical system (dependent on aperture) and the solid angle subtended by the pixel's "field-of-view", :math:`\Omega_p`.  For now, we can assume the field-of-view of a given pixel is simply the camera's field of view, divided by the number of pixels.  Secondly, we're assuming circular aperture-stop and so given an aperture diameter of :math:`d`, the collection area is simply :math:`A = \pi \left(\frac{d}{2}\right)^2`.  Thus giving us:

.. math::
    \Phi_{p}(\lambda)= \Omega_p \cdot \pi \left(\frac{d}{2}\right)^2 L_p(\lambda)

To go from the received spectral power for a given pixel, :math:`\Phi_p(\lambda)`, to the received spectral energy for a given pixel, :math:`Q_p(\lambda)`, we simply multiply by the sensor's integration time (also commonly referred to as shutter speed, or exposure time).  Given an integration of time of :math:`t`, that gives us:

.. math::
    Q_p(\lambda) = t \cdot \Phi_p(\lambda)

So we can now calculate the number of photons per wavelength incident on our pixel by using the definition of photon energy given in Eq :eq:`photon-energy-lam`.  To calculate the number of photons of a given wavelength, :math:`n_p\left(\lambda\right)`, we simply compute:

.. math::
    n_p\left(\lambda\right) = \frac{Q_p(\lambda)}{\frac{hc}{\lambda}}

Once we have the number of photons per wavelength, we use the sensor's Quantum Efficiency, :math:`Q_E(\lambda)`, to calculate the number of electrons generated in response to the number of photons.  We do this separately for each wavelength, and combine the number of electrons, giving us an electrical signal with :math:`n_e` electrons.

.. math::
    n_e = \sum_{\lambda} Q_E(\lambda) \cdot n_p(\lambda)

.. note::
    This process loses any information about the wavelength of light that excited the pixel.  In one-shot-color cameras, this is overcome by covering adjacent pixels in red, green, or blue filters.  This makes a pixel to effectively only be sensitive to a specific range of wavelengths, and adjacent pixels can then be combined in a process known as "de-mosaicing" to produce a single ColorRGB color value in the final image.  Alternatively, the entire sensor could be covered by a flter that only transmits a particular wavelength allowing you to make the sensor selectively sensitive to particular band.  In astronomy, we often use narrow bandpass filters around specific physically useful frequencies, such as the Hydrogen-:math:`\alpha`, OxygenIII, or SulfurII emission lines.

At this point, we need to account for both the limitations of the sensor as well as the read-out process itself.  Digital sensors are only capable of storing a specific number of electrons.  This is known as the *well depth*, and once a pixel has collected more electrons than that, the pixel is fully exposed.  It can even start bleeding electrons into nearby pixels as it "overflows", which can cause over-exposed parts of an image to appear to bled over into adjacent pixels.  Additionally, there are noise sources present in any electrical system.  In the case of camera sensors, we typically have dark-noise and read-out noise.  Dark-noise causes pixels to fill with electrons simply from the heat (thermal energy) of the sensor.  Read-out noise occurs when the pixels are actually read from the sensor and can introduce a random number of electrons to the final signal.  Note that, the dark-noise grows with exposure time as it is occuring within the actual photosensitive cells, while read-out noise is independent of exposure time as it occurs only at the time of actually reading out the charges from each pixel.

These processes become even more complicated by the fact that we now pass this electrical signal through an Analog-to-Digital Converter (ADC), and apply a gain.  The gain is specificed with units of :math:`\frac{ADU}{e^-}`, where ADU stands for "Analog-to-Digital Units".  The ADC will then convert some number of electrons into a digital value witha certain amount of precision.  A 16-bit ADC will be able to output 65,535 different values.  If a pixel has a well depth of 10,000 electrons, in order to use the enire dynamic range of the sensor, we would use a gain of :math:`6.5535 \frac{ADU}{e^-}`, so that a fully saturated pixel yields the maximum posible value.

.. note::
    Be aware, that we may not want to use these gains.  If we are observing something particularly dim, that does not come close to saturating the sensor, we'd want to use a higher gain to ensure we have the maximum dynamic range.  For example if we have a well depth of 10,000 electrons, a 16-bit ADC (65,535 different values), yet are observing somethingthat only produces a maximum response of 1,000 electrons, we should use a gain of :math:`65.535\frac{ADU}{e^-}` to ensure the brightest part of our image is a fully saturated pixel.  But be aware that gain scales both the true signal (electrons generated by photons hitting the sensor) as well as the noise signals.  So a higher gain with a poor Signal-to-Noise Ratio (SNR) will not help, and will make noise artifacts appear worse.

Given a gain :math:`G`, an ADB bit-depth of :math:`N`, a true signal of electrons :math:`n_e`, a dark noise signal of :math:`n_{d,e}`, and a read-out noise signal of :math:`n_{r,e}`, we calculate the final pixel value :math:`I`:

.. math::
    I = \frac{G \cdot\left(n_e + n_{d,e} + n_{r,e}\right)}{2^N - 1}


Simplification for ColorRGB Rendering
-------------------------------------

In the previous section we saw that camera sensors are not capable of distinguishing between specific wavelengths.  In reaity, one-shot-color cameras get around this by using a filter mosaic (such as the bayer filter) to make each pixel sensitive to a specific wavelength of a light, and a final ColorRGB value can be reconstructed from adjacent pixels.  For many rendering applications, we'd much rather to just deal directly with ColorRGB rendering (because we, in a simulated world, *can* differentiate between wavelength!)

.. warning::
    I have not yet written documentation for this yet.