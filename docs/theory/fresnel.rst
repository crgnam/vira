Fresnel Equations
======================================================

.. warning::
    This page is INCOMPLETE

When light intersects the interface between two mediums, the Fresnel equations give us the proportion of light that is reflected and the proportion of light that is refracted into a medium.  \

Given an angle of incidence of :math:`\theta_i`, and an angle of refraction of :math:`\theta_t` (calculated via Snell's law), the reflectance of S-polarized light is given by:

.. math::
    R_s = \left\lvert \frac{n_1\cos{\theta_i} - n_2\cos{\theta_t}}{n_1\cos{\theta_i} + n_2\cos{\theta_t}} \right\rvert^2

while the reflectance of the P-polarized light is given by:

.. math::
    R_p = \left\lvert \frac{n_1\cos{\theta_t} - n_2\cos{\theta_i}}{n_1\cos{\theta_t} + n_2\cos{\theta_i}} \right\rvert^2

Because of the conservation of energy, the transmitted power in each case can simply be found with:

.. math::
    \begin{align}
    T_s &= 1 - R_s\\
    T_p &= 1 - R_p
    \end{align}

Schlick's approximation
-----------------------

Because evaluating the full Fresnel equations can be expensive we can use Schlick's approximation.  Schlick's approximation is much faster to evaluate:

.. math::
    R(\theta_i) = R_0 + (1-R_0)(1-\cos{\theta_i})^5

where :math:`R_0` is the base reflectivity, given by:

.. math::
    R_0 = \left(\frac{n_1 - n_2}{n_1 + n_2}\right)^2

.. note::
    Schlick's approximation does *not* take into account separate polarization components.