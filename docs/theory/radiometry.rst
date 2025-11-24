Radiometry
===============================================

Radiometric Quantities
----------------------

*Vira uses* radiometric quantities for its calculations.  As a quick review, we'll briefly introduce some of the basic radiometric quantities:

.. _radiometric-table:

.. list-table:: Radiometric Quantities
   :widths: 10 6 6 50 10
   :header-rows: 1

   * - Quantity
     - Symbol
     - Units
     - Description
     - Source
   * - Radiant Flux
     - :math:`\Phi_e`
     - :math:`W`
     - The radiant energy emitted, reflected, transmitted, or received per unit time.  Sometimes called "Radiant Power".
     - `source <https://en.wikipedia.org/wiki/Radiant_flux>`__
   * - Irradiance
     - :math:`E_e`
     - :math:`\frac{W}{m^2}`
     - The radiant flux received by a surface per unit area.  Sometimes called "Intensity", or "flux of radiant energy".
     - `source <https://en.wikipedia.org/wiki/Irradiance>`__
   * - Radiant Exitance
     - :math:`M_e`
     - :math:`\frac{W}{m^2}`
     - The radiant flux emitted by a surface per unit area.  Sometimes called "Radiant Emittance".
     - `source <https://en.wikipedia.org/wiki/Radiant_exitance>`__
   * - Radiant Intensity
     - :math:`I_{e,\Omega}`
     - :math:`\frac{W}{sr}`
     - The radiant flux emitted, reflected, transmitted or received, per unit solid angle.  Sometimes called "Intensity".
     - `source <https://en.wikipedia.org/wiki/Radiant_intensity>`__
   * - Radiance
     - :math:`L_{e,\Omega}`
     - :math:`\frac{W}{sr \cdot m^2}`
     - The radiant flux emitted, reflected, transmitted or received by a given surface, per unit solid angle per unit projected area.
     - `source <https://en.wikipedia.org/wiki/Radiance>`__

.. warning::

   Many of these quantities are sometimes called "Intensity" in different circumstances.  This can lead to confusion as none of the quantities are the same, and so the use of the term "intensity" is ambiguous.  For this reason, *Vira* avoids using the term intensity, and we urge you to avoid using it as well!


Spectral Quantities
----------------------

All of these radiometric quantities also have related spectral counterparts.  That is, the quantity per unit wavelength (or frequency, depending on how the spectrum is defined).  The below presents the units using standard SI base units, however it is common when dealing with visible light to use :math:`nm` as the unit of wavelength as opposed to :math:`m`.

.. _spectral-table:

.. list-table:: Corresponding Spectral Quantities
   :widths: 10 6 6 50 10
   :header-rows: 1

   * - Quantity
     - Symbol
     - Units
     - Description
     - Source
   * - Spectral Flux
     - :math:`\Phi_{e,\nu}`
     - :math:`\frac{W}{m}`
     - The radiant flux per unit frequency.  Sometimes called "Spectral Power".
     - `source <https://en.wikipedia.org/wiki/Radiant_flux>`__
   * - Spectral Irradiance
     - :math:`E_{e,\nu}`
     - :math:`\frac{W}{m^2 \cdot m}`
     - The irradiance of a surface per unit frequency.
     - `source <https://en.wikipedia.org/wiki/Irradiance>`__
   * - Spectral Exitance
     - :math:`M_{e,\nu}`
     - :math:`\frac{W}{m^2 \cdot m}`
     - The radiant exitance of a surface per unit frequency.  Sometimes called "Spectral Emittance".
     - `source <https://en.wikipedia.org/wiki/Radiant_exitance>`__
   * - Spectral Intensity
     - :math:`I_{e,\Omega,\nu}`
     - :math:`\frac{W}{sr \cdot m}`
     - The radiant intensity per unit frequency.
     - `source <https://en.wikipedia.org/wiki/Radiant_intensity>`__
   * - Spectral Radiance
     - :math:`L_{e,\Omega,\nu}`
     - :math:`\frac{W}{sr \cdot m^2 \cdot m}`
     - The radiance of a surface per unit frequency.
     - `source <https://en.wikipedia.org/wiki/Radiance>`__


Photon Energy
-------------

Recall that the energy of a single photon is given by:

.. math::
    :label: photon-energy-freq

    E = h \nu

where :math:`h` is Planck's constant, and :math:`\nu` is the frequency of the photon.  In wavelength form, it is given as:

.. math ::
    :label: photon-energy-lam

    E = \frac{hc}{\lambda}

where :math:`\lambda` is the wavelength of the photon, and :math:`c` is the speed of light in vacuum.

This means that, given a spectral power and a duration (such as an exposure time of our camera), we can calculate a corresponding number of photons.  For each desired wavelength, simply obtain the radiant power (units of :math:`W = \frac{J}{s}`) and multiply by the duration in seconds to obtain the power.  Then, simply divide by the energy per photon of that particular wavelength to arrive a photon count.

This process can be useful for making use of `Quantum Efficiency <https://en.wikipedia.org/wiki/Quantum_efficiency>`__ in sensors.

.. note::
    For the remainder of *Vira* documentation while we may use the term "radiometry" we'll typically be referring to the spectral form, as for many real-world materials and light-sources, the radiometric quantites do vary with wavelength.

        


Radiometric Relations
---------------------
.. math::
    \begin{align}
    I_{e,\Omega} &= \frac{\partial\Phi_e}{\partial\Omega}\\
    M_e &= \frac{\partial \Phi_e}{\partial A} \\
    L_{e,\Omega} &= \frac{\partial^2 \Phi_e}{\partial\Omega \partial (A\cos{\theta})}\\
    \end{align}

.. note::

    Radiance remains constant along a ray (so long as it is not attenuated by a medium such as air).  This may seem counterintuitive at first as we typically think of objects becoming less bright as they get further away (per the inverse square law).  While it is true that Irradiance (:math:`$E_e`) is inversely proportional to the distance to the light sourse squared (:math:`E_e \propto \frac{1}{r^2}`), remember that Radiance (:math:`L_{e,\Omega}`) also depends on the solid angle subtended by the light source.  And the solid angle subtended by a light source is also inversely proportional to the distance squared.  These two effects cancel out, and so radiance is conserved.

Example with Solar Radiance
---------------------------
The reason that ray tracers deal with radiance, is because radiance (so long as not attenuated by a medium such as air or glass) is constant along a ray.  So for example we can calculate the radiance in a ray of sunlight by either looking at the radiant exitance of the sun, or at the irradiance received by our camera.  The conservation of radiance ensures we should receive the same value.

First, lets assume the following values:

.. math::
    \begin{align}
    \Phi_e &= 3.7889e26 \ W \\
    R_{sun} &= 6.957\times10^8 \ m\\
    R_{earth} &= 6.3781\times 10^6 \ m\\
    d &= 1.4776\times10^11 \ m
    \end{align}

Then the irradiance at Earth can be calculated by applying the inverse square law to the total radiant power of the Sun, yielding:

.. math::
    E_e = 1.381\times10^3 \ \frac{W}{m^2}

Using the equation to calculate the `solid angle of distant spherical object <https://en.wikipedia.org/wiki/Solid_angle#:~:text=Celestial%20objects%5Bedit%5D>`__:

.. math::
    \Omega = 2\pi\left(1 - \frac{\sqrt{d^2 - R^2}}{d}\right)

we can find that the solid angle subtended by the Sun from the perspective of the Earth is:

.. math::
    \Omega_{sun} = 6.9644\times10^-5 \ sr

Therefore, we can now calculate the radiance by simply dividing the irradiance received at the Earth, by the solid angle of the Sun:

.. math::
    \begin{align}
    L_{e,\Omega} &= E_e/\Omega_{sun}\\
    &= \frac{1.381\times10^3 \ \frac{W}{m^2}}{6.9644\times10^-5 \ sr} \\
    &= 1.9829\times10^7 \frac{W}{sr\cdot m^2}
    \end{align}

To now calculate the radiance from light leaving the sun, we calculate the radiant exitance by dividing the total radiant power of the Sun by the surface area of the sun, yielding:

.. math::
    M_e = 6.2296\times10^7 \ \frac{W}{m^2}

Now, to solve for the radiance, we first need to derive the relation between radiance and radiant exitance.  Using the basic relations above, we can easily find this:

.. math::
    \begin{align}
    L_{e,\Omega} &= \frac{\partial M_e}{\partial \Omega} \frac{1}{\cos{\theta}}\\
    \int_SL_{e,\Omega} \cos{\theta} \ d\Omega &= M_e
    \end{align}

If we assume that the sun is an isotropic emitter (i.e. a Lambertian emitter) than we can integrate over the entire hemisphere of a point on the sun, giving:

.. math::
    \begin{align}
    M_e &= \int_0^{2\pi}\int_0^{\pi/2} L_{e,\Omega}\cos{\theta}\sin{\theta} \ d\theta \ d\phi = L_{e,\Omega} \pi
    \end{align}

And so we find the radiance to be:

.. math::
    \begin{align}
    L_{e,\Omega} &= M_e/\pi\\
    &= \frac{6.2296\times10^7 \ \frac{W}{m^2}}{\pi \ sr} \\
    &= 1.9829\times10^7 \frac{W}{sr\cdot m^2}
    \end{align}