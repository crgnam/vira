Polarization
===============================================

.. warning::
    This page is INCOMPLETE

Generally speaking, we could add a field to `vira::Ray` structs which store a `Stokes vector <https://en.wikipedia.org/wiki/Stokes_parameters>`__.  When computing light interactions, we could then use `Mueller Calculus <Mueller calculus>`__ to keep track of polarizing effects.

While this should work reasonably well for specular reflections (and possibly pure diffuse reflections), there are problems both with limitations of the microfacet model and with subsurface scattering.  Subsurface scattering is a much more complicated phenomenon to model, and microfacet models do not take into account multiple bounces within the microfacet structure, which tends to have a depolarizing effect.

S and P polarizations
---------------------

We can fully describe any polarization state as a combination of two orthogonal linear polarizations.  S-polarization refers to polarization that is *normal* to the plane of incident (where plane of incidence is the plane which contains both the surface normal and propagation vector of the light).  P-polarization, as it is orthogonal, is polarization *in* the plane of incidence.  Unpolarized light will have an equation amount of power distributed between both the S and P polarization components.