Rendering Equation Revisited
===============================================

Recall the *Rendering Equation* originally presented in Eq :eq:`rendering-equation`:

.. math::
    L_o(\vec{x},\omega_o,\lambda) = L_e(\vec{x},\omega_o,\lambda) + \int_{\Omega} f_r(\vec{x},\omega_i,\omega_o,\lambda)L_i(\vec{x},\omega_o,\lambda)(\omega_i\cdot \hat{n}) \ d\omega_i


Integrating Lambertian BRDF
---------------------------

Lets consider a simple scene with no emittance, and so the rendering equation is simply:

.. math::
    L_o(x, \omega_o) = \int_{\Omega} f(x,\omega_o,\omega_i)L_i(x,\omega_i) \left(\omega_i \cdot \hat{n}\right) \ d\omega_i

Now, consider a surface with a Lambertian BRDF (see: :doc:`brdf`) 

.. math::
    f_r = \frac{\rho}{\pi}

where :math:`\rho` is the surface's albedo.  With this constant BRDF, the integral simply becomes:

.. math::
    L_r = \frac{\rho}{\pi}\int_{\Omega}L_i(\vec{x},\omega_i) \left(\omega_i \cdot \hat{n}\right) \ d\omega_i

We'll also have an isotropic spherical light source (see :doc:`lights`), with a radius :math:`R` and a radiant power of :math:`\Phi`, that is :math:`d` away from point :math:`x` directly along the surface normal vector :math:`\hat{n}`.  Because the light source emits light in all directions, :math:`L_i` has no dependence on the direction :math:`\omega_i`, and so it can be pulled out from the integral as well:

.. math::
    L_r = \frac{\rho}{\pi} L_i \int_{\Omega} \left(\omega_i \cdot \hat{n}\right) \ d\omega_i


Given that the light is located directly above the point, along :math:`\hat{n}`, then all of the light incident on :math:`\vec{x}` is within :math:`\delta` of :math:`\hat{n}`, where :math:`\delta` is the half the angular diameter of the light.  The angular diameter is defined as:

.. math::
    \delta = \sin^{-1}\left(\frac{R}{d}\right)

Therefore, our integral can be rewritten as:

.. math::
    \begin{align}
    L_r &= \frac{\rho}{\pi} L_i \int_0^{2\pi} \int_0^{\delta} \cos{\theta_i}\sin{\theta_i} \ d\theta d\phi\\
    &= \frac{\rho}{\pi} L_i \int_0^{2\pi} \left[\frac{-\cos^2{\theta}}{2}\right]_0^\delta  \ d\phi\\
    &= \frac{\rho}{\pi} L_i \left(\pi \sin^2\left(\sin^{-1}\frac{R}{d}\right)\right)\\
    &= \rho L_i \frac{R^2}{d^2}
    \end{align}

Now lets say I want to setup a monte carlo estimator, which for this setup should take the following form:

.. math::
    L_r \approx \frac{\rho}{\pi} \frac{1}{N} \sum_{i=1}^N \frac{L_i(\omega_i)\cos{\theta_i}}{p(\omega_i)} 

If we were to uniformly sample :math:`\omega_i` from the entire hemisphere, then :math:`p(\omega_i) = \frac{1}{2\pi}` and so the estimator simplifies to:

.. math::
    L_r \approx 2\rho \frac{1}{N} \sum_{i=1}^N L_i(\omega_i)\cos{\theta_i} 

If instead, we were to use importance sampling and directly sample the light, then :math:`p(\omega_i) = \frac{1}{\Omega}` where :math:`\Omega` can be calculated using:

.. math::
    \Omega = 2\pi\left(1 - \frac{\sqrt{d^2 - R^2}}{d}\right)

and so the estimator becomes:

.. math::
    L_r \approx \frac{\rho}{\pi} \frac{\Omega_{light}}{2\pi} \frac{1}{N} \sum_{i=1}^N L_i(\omega_i)\cos{\theta_i} 



If we assume :math:`\Phi = 100W`, :math:`d = 100m`, :math:`R = 10m`, and :math:`\rho = 0.4`, our analytical solution yields:

.. math::
    L = 1.0132\times10^{-4} \frac{W}{sr\cdot m^2}

If we use the uniformly sampled monte carlo method with a million samples, we get:

.. math::
    L_{uniform} = 1.0141\times10^{-4} \frac{W}{sr\cdot m^2}

And if I use the importance sampled version with 10 samples, we get:

.. math::
    L_{importance} = 1.0131\times10^{-4} \frac{W}{sr\cdot m^2}


Hopefully it is clear how valuable importance sampling is for quickly and accurately evaluating the rendering equation.  The goal of this document wasn't to necessarily make you an expert in the topic, but to give you decent insight into the motivation of how rendering has been implemented in *Vira*.