Bidirectional Reflectance Distribution Function (BRDF)
======================================================

A Bidirectional Reflectance Distribution Function (BRDF) defines how light is reflected off of a surface, without accounting for internal scattering or transmittance.  The function takes as arguments the direction to the light source (:math:`\omega_i`), and the outgoing direction of the light (:math:`\omega_o`).  It then returns the ratio of the reflected radiance (:math:`L`) along the direction :math:`\omega_o` to the irradiance (:math:`E`) incident on the surface from the direction :math:`\omega_i`.  This means that the evaluation of the BRDF has units of :math:`sr^{-1}`.

For a BRDF to be physically based, it must obey three properties:

#. Positivity: :math:`f_r(\omega_i, \omega_o) \geq 0`

#. Helmhotz Reciprocity: :math:`f_r(\omega_i, \omega_o) = f_r(\omega_o, \omega_i)`

#. Energy conservation: :math:`\int_{\Omega} f_r(\omega_i, \omega_o) \left(\omega_o \cdot \hat{n}\right) d\omega_o \leq 1`



Lambertian BRDF
---------------
As an example, lets consider the simplest physically based BRDF: The Lambertian BRDF.  The Lambertian BRDF is a diffuse reflectance model that is invariant to the viewing direction meaning that it reflects the same radiance in all directions.  We'll now briefly derive it to get a sense of how it works.

To start, let us define the *albedo* (:math:`\rho`) as being the proportion of incident light that is diffusely reflected by a surface.  Therefore, *albedo* can be defined as:

.. math::
    :label: albedo-definition

    \rho = \int_{\Omega} f_r(\omega_i, \omega_o)\left(\omega_i \cdot \hat{n}\right) d\omega_i

That is to say, if we consider the sum of the incident light from all possible directions :math:`\omega_i \in \Omega`, and evaluate the BRDF for a given observer direction :math:`\omega_o`, the result is the albedo (:math:`\rho`).  Because we are after some :math:`f_r(\omega_i, \omega_o)` that is invariant to the viewing direction, we know that it must be a constant value and so can be pulled out of the integral:

.. math::

    \rho = f_r \int_{\Omega}\left(\omega_i \cdot \hat{n}\right) d\omega_i

We can now rewrite things to express the BRDF in terms of the albedo (which is often known for a given material):

.. math::
    :label: lambertian-definition

    f_r = \frac{\rho}{\int_{\Omega}\left(\omega_i \cdot \hat{n}\right) d\omega_i}

Integrating over the solid angle is most easily done by first reparameterizing into spherical coordinates.  That is to say, we can rewrite:

.. math::

    \int_{\Omega}\left(\omega_i \cdot \hat{n}\right) d\omega_i = \int_0^{2\pi}\int_0^{\pi/2} \left(\omega_i \cdot \hat{n}\right) \sin{\theta} \ d\theta d\phi

where :math:`\theta` is the angle away from the surface normal (:math:`\hat{n}`), and :math:`\phi` is the azimuth angle around the normal.  Notice then, that we can simply rewrite :math:`\left(\omega_i \cdot \hat{n}\right)` as :math:`\cos{\theta}`.  Substituting this into our new spherical integral gives us a trivial integral to evaluate:

.. math::

    \int_0^{2\pi}\int_0^{\pi/2} \cos{\theta} \sin{\theta} \ d\theta d\phi = \pi

Substituting this into Eq :eq:`lambertian-definition` gives us the trivial (and expected) lambertian BRDF:

.. math::
    f_r = \frac{\rho}{\pi}


Microfacet-based BRDFS
----------------------
Although it may not seem like it, in reality, *all* reflections are specular.  Things appear to reflect light diffusely only because, at a microscopic level, their surfaces are rough.  Thus even though at any given point the light is specularly reflected, the local normal at that point may be pointed in a wide range of directions, and so the reflection at any given point may not be specular on a macroscopic scale.  Simple BRDFs such as the lambertian model simply approximate this behavior macroscopically, but this makes it difficult to then account for other behaviors such as polarization.  Even ignoring polarization, simple BRDFs can produce unrealistic results.

Microfacet-based models improve upon this, by directly modeling the microscopically rough nature of a surface as a collection of "microfacets".  These microfacets are not actually part of your geometry, but you can think of them conceptually as helping us determine a new local normal, with which we can compute a specular reflection.  If we define a material to be perfectly smooth, all of these microfacets are aligned with the true surface normal, and so we'll get specular reflections as we expect.  If we define a material to be rough, we'll see the microfacet normals distributed all around the local normal, and so we'll get reflections which appear diffuse.  Because we are representing the surface roughness continuously from 0 to 1, any value inbetween will appear "glossy", which indeed is what we would intuitively expect a material to look like.

This topic is rather large however, so we will leave it for a new section: :doc:`microfacets`.