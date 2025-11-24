Microfacet BSDFs
======================================================

Microfacet Models are based on the assumption that all reflections are perfectly specular, and that materials only appear to reflect diffusely because on a microscopic scale, their surfaces are rough and so specularly reflect light in all different directions.  This is largely true in reality, and so is a good model to use.  In addition, these models use the fresnel equations to determine the proportion of incident light that is reflected off of a surface, and the proportion that is transmitted (refracted) into the surface.  What happens to that light transmitted into the surface, depends on if the material is metallic, or dielectric.

.. note::
    In computer graphics we use the term Metallic to refer to anything that is conductive (gold, copper, silver, etc.), and dielectrics to refer to anything that is an insulator (plastic, stone, wood, etc.)

Dielectrics
-----------

For electrical insulators, light that is transmitted into the material can continue propagating. If the atoms of the medium do not resonate with the frequency of light, the EM waves can propagate through and the medium is transparent to that wavelength.  If the medium has an irregular crystal structure with internal surfaces (such as grain boundaries), there are secondary fresnel reflections/refractions at each interface, resulting in subsurface scattering. When this does not penetrate deep into the surface, the re-emitted light appears to have simply be reflected diffusely.

Metallic
--------

Metals (conductors) have many free electrons, which are free to ocillate and thus deplete incoming radiation of its energy.  This means that metals are extremely absorptive to light that refracts into them.  This means that light transmitted into a metal is immediately absorped and so cannot be scattered back out diffusely, and so metals *only* exhibit specular reflections.  Additionally, some metals absorb light of specific wavelengths regardless, reducing the amount that gets reflected at all.  This means that colored metals (such as gold) tint the specularly reflect light with the color of the metal.

.. warning::
    I need to formulate a more clear explanation of this interaction