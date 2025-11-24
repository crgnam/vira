Lights
======

A light is defined by having a Radiant Power (:math:`\Phi`) (or Spectral power, if it varies with wavelength.  See :doc:`radiometry`).  For area lights (that is, lights with a physical size), we pre-calculate their emitted radiance:

.. math::
    L = \frac{\Phi}{A\Omega}

where :math:`A` is their surface area, and :math:`\Omega` is the total solid angle of directions the light could be emitting into.  For a non-directional light, that means :math:`\Omega=4\pi`.

The emission of light into a scene is what allows anything to be seen at all.  The very simplest implementation of a path tracer simulates light by tracing rays from an observer (typically a camera) around a scene, until a light source is intersected.  Doing this many times, allows us to estimate the full integral present in Eq :eq:`rendering-equation`, the *Rendering Equation*, via monte carlo estimation. :doc:`monte_carlo`

Naively using the basic monte carlo estimator, where samples are taken uniformly over the hemisphere, is not ideal.  Particularly in the vacuum of space, *most* directions a sample ray could head off into will be completely devoid of light.  We are much more interested in the (relatively few) directions which carry large amounts of radiance.  As you may recall, this is the *importance sampling* modification of the monte carlo method.  That is, when sampling lights, we can generate rays which we know will intersect a light (and thus have high radiance contributions), and then weight them by their probability of having been sampled by considering the angular size of the light.


Spherical Light
---------------

A spherical light subtends some solid angle :math:`\Omega` given by:

.. math::
    \Omega = 2\pi \left(1-\frac{\sqrt{d^2-R^2}}{d}\right)

where :math:`d` is the distance to the light and :math:`R` is the radius of the light.  When we go to sample this light, what we want to do is uniformly sample the possible directions to the light.  Note, this is NOT the same as sampling points *on* the light.  While these may at first seem similar in concept, it will actually introduce a bias into your radiance computations, as our assumption is that the PDF remains constant over the solid angle subtended by the light.  There are many derivations of how this sampling process can be done, but as an example, we can use a method similar to this:

* Uniformly sample an azimuth angle :math:`\phi` from the domain :math:`[0, 2\pi)`

* Uniformly sample some variable :math:`\zeta_{\theta}` from the domain :math:`[0,1)`, and calculate a seed elevation angle using: 

.. math::
    \theta = \cos^{-1}\left(1 - \zeta_{\theta} + \zeta_{\theta}\sqrt{1 - \left(\frac{r}{d}\right)^2}\right)

* Use :math:`\theta` and :math:`\phi` to compute a direction, and rotate this new direction into the correct tangent space (as the direction we've just calculated assumes :math:`\hat{n} = \left[0 \ 0 \ 1\right]^T`, which likely does not align with the true surface normal vector we're trying to evaluate.

When performing monte carlo importance sampling, we use a PDF defined by:

.. math::
    p(\omega_i) = \frac{1}{\Omega}


Point Light
-----------

Point lights are physically impossible, and cannot be thought of in-terms of radiance.  For a light with physical size, we can compute the radiance it emits by taking its total radiant power, and dividing by both its surface area as well as the total solid angle of directions into which it is emitting that light.  This is not possible for a point light, as it has zero size!  Because of this, we can only represent it radiant intensity, which is described as the radiant power divided by the total solid angle of directions into which it is emitting:

.. math::
    I = \frac{\Phi}{4\pi}

This has units of :math:`\frac{W}{sr}`.  When sampling the light, we then apply the inverse square law (by dividing by the distance to the light squared) to obtain the radiance received.  For monte carlo importance sampling, we simply use a PDF of 1, as we are just directly sampling the entireity of the light, and not actually integrating.


.. note::
    In the future, we would like to support Directional Lighting with a disc sampler (to approximate a distant sun), Spot Lighting (for something like a flash-light or other light that only emits in a particular direction), and rectangular area lights.


Black Body Radiation
--------------------
For some applications, it may be suitable to simply define a uniform spectral radiance for a light-source.  Other times, we may have a specific spectral radiance curve we wish to use.  However, other times, it may be beneficial to use Planck's law of blackbody radiation.  This is especially useful when dealing with light sources such as the Sun.  Planck's law can be defined as:

.. math::
    \Phi_{\nu}(T) = \frac{2h\nu^3}{c^2} \frac{1}{e^\frac{h\nu}{kT} - 1}

where :math:`\Phi_{\nu}(T)` is the spectral radiance, :math:`h` is the Planck constant, :math:`c` is the speed of light in a vacuum, :math:`k` is the Blotzmann constant, and :math:`\nu` is the frequency of the electromagnetic radiation, and :math:`T` is the absolute temperature of the emitting body.  It can also be expressed in terms of wavelength instead of frequency:

.. math::
    Phi_{\lambda}(T) = \frac{2hc^2}{\lambda^5}\frac{1}{e^\frac{hc}{\lambda kT} - 1}

where :math:`\lambda` is the wavelength of the electromagnetic radiation. 