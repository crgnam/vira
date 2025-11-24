Rendering Equation
===============================================

Recall the radiometric quantities outlined in :ref:`radiometric-table`, and their spectral counterparts outlined in :ref:`spectral-table`.  Using these quantities we can define an equation which relates in the incoming and outgoing radiance from a point.  This is known as the *Rendering Equation*.  Using the spectral form of our radiometric quantities (as to capture wavelength dependent behaviors) it would be written as follows:

.. math::
    :label: rendering-equation

    L_o(\vec{x},\omega_o,\lambda) = L_e(\vec{x},\omega_o,\lambda) + \int_{\Omega} f_r(\vec{x},\omega_i,\omega_o,\lambda)L_i(\vec{x},\omega_o,\lambda)(\omega_i\cdot \hat{n}) \ d\omega_i

where:

* :math:`\vec{x}` is the point at which we're evaluating

* :math:`\omega_o` is the direction of the outgoing light

* :math:`\omega_i` is the direction towards the incoming light (negative the direction the light is traveling)

* :math:`\lambda` is a specific wavelength

* :math:`L_e(\vec{x},\omega_o,\lambda)` is the emitted spectral radiance from point :math:`\vec{x}` in direction :math:`\omega_o`

* :math:`L_o(\vec{x},\omega_o,\lambda)` is the total output spectral radiance from point :math:`\vec{x}` in direction :math:`\omega_o`

* :math:`L_i(\vec{x},\omega_i,\lambda)` is the incoming spectral radiance at point :math:`\vec{x}` from direction :math:`\omega_i`

* :math:`f_r(\vec{x},\omega_i,\omega_o,\lambda)` is the Bidirectional Reflectance Distribution Function (BRDF) relating how the light from :math:`\omega_i` is reflected to :math:`\omega_o` for a given wavelength (:math:`\lambda`)

* :math:`\hat{n}` is the surface normal vector at point :math:`\vec{x}`

* :math:`\Omega` is the unit hemisphere at point :math:`\vec{x}` such that :math:`\Omega = \{\omega_i \ | \ \omega_i\cdot\hat{n} > 0 \}`

While *Vira* does aim to support fully spectrally based path tracing, most use-cases will use Trichomatic (e.g. ColorRGB), in which case simplified versions of the BRDF can be used, and the dependence on wavelength disappears.  Note, in Eq :eq:`rendering-equation` we are also ignoring time dependence.

Further, the radiance emitted from a surface (:math:`L_e(\vec{x},\omega_o,\lambda)`) is trivial to account for but is only applicable to objects which are actively emitting light.  For many of *Vira*'s use-cases, this will not be applicable, as it implies the object being rendered is glowing, while this could be achieved by specifying an emission texture, we can ignore it for now.

.. note::

    There are some limitations to how the rendering equation is presented here.  Most relevant to *Vira* is that, as written, Eq. :eq:`rendering-equation` cannot account for polarization, transmission, or doppler effects.  This can be over-comer in the future, namely by replacing the BRDF with a Polarimetric Bidirectional Scattering Distribution Function (BSDF), which accounts for both reflectance and transmittance of a material (transmittance would require integrating over a second hemisphere, pointed into the material).  Similarly, by using a microfacet based models, it can be possible to model polarization effects.  This is not yet implemented in *Vira*, and so these limitations are something to be aware of.

In all but the most contrived examples, this integral cannot be evaluated exactly, and so we must employ the monte carlo methods presented in :doc:`monte_carlo`.