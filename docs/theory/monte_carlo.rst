Monte Carlo Estimation
===============================================

Say we have some function :math:`f(x)` that we wish to integrate.  If we are not able to analytically integrate :math:`f(x)`, we must restort to numerical methods.  We may be inclined to try numerical quadrature methods, and perform the integration as a finite sum of small slices of the function.  This will work for simple functions, but for functions with high dimensionality (as is common in path tracing), this quickly becomes infeasible.

Instead of looking at every possible point on the function, we could instead randomly sample the function and then combine these samples to form an estimate of what the integral should be.  This approach is known as monte carlo estimation, and is loosely formalized as:

.. math::
    :label: monte-carlo-definition

    F = \int_S f(x) dx \Rightarrow F \approx \frac{1}{N} \sum_{i=1}^N \frac{f(x_i)}{p(x_i)}

In plane english, this tells us that we can approximate the integral of :math:`f(x)` by calculating a weighted sum of a set of :math:`N` random samples, each with a probability density function (PDF) of :math:`p(x_i)`.

Uniform Sampling Example
------------------------

Lets consider a scenario where we wish to integrate the following function:

.. math::
    
    f(x) = e^x - 1

between 0 and 2.  Obviously, we can very trivially find the anti-derivative of this function to be:

.. math::
    \begin{align}
    F(x) &= \int_0^2 e^x - 1 \ dx\\
    &= \left(e^x -x\right)|_0^2\\
    &\approx 4.3891
    \end{align}

but lets assume for a moment that we are unable to compute this, and wish to use a monte carlo estimator.  

To begin, lets uniformly sample values :math:`x` from the domain :math:`[0, 2)`.  We know that the probability that we'll select some number within the range 0 to 2, is 100% (because that is the domain we are sampling from!).  We can write that as:

.. math::
    \int_0^2 p(x) dx = 1

Additionally, because we are uniformly sampling the domain of :math:`[0,2)`, we know that the selection of any :math:`x_i` is just as likely as the selection of any other :math:`x_j`, which means that the PDF is constant (:math:`p(x) = c`).  Therefore we can solve for what the probability must be:

.. math::
    \begin{align}
    \int_0^2 p(x) dx &= 1\\
    c\int_0^2 dx&= 1\\
    c \ x|_0^2 &= 1\\
    c &= \frac{1}{2}
    \end{align}

Plugging this constant probability into Eq. :eq:`monte-carlo-definition` gives us the following basic monte carlo estimator:

.. math::
    F \approx \frac{2}{N} \sum_{i=1}^N f(x_i)

We can now run this for :math:`N=100` samples, and we obtain the value of 4.5707.  This is only about 4% different than the analytically known true value!


Importance Sampling
-------------------

In the previous example, notice that the function :math:`f(x) = e^x - 1` is near 0 when :math:`x` is small, and grows exponentially as :math:`x` increases.  This poses a problem for our monte carlo estimator, as since we are uniformly sampling the domain, we are wasting many samples at points when :math:`f(x)` is small.  We should be trying to sample more where we know :math:`f(x)` is larger, as those points will contribute more to the final integral.  But how can we do that?

We don't need to uniformly sample the domain.  We can sample any function we like!  But sampling from a function with a similar shape to :math:`f(x)` will lower the variance of our estimator.  Given that :math:`f(x)` is small when :math:`x=0`, and grows larger exponentially, lets try to sample from a function proportional to :math:`x^2`, as this will exhibit the same qualitative behavior.

But how do we sample from an arbitrary function?  The key is to utilize `Inverse transform sampling <https://en.wikipedia.org/wiki/Inverse_transform_sampling>`__.  We know that we want a PDF :math:`p(x) \propto x^2`, and we can find this function by simply finding a normalization factor :math:`c` that ensures it integrates to 1 over our domain:

.. math::
    \begin{align}
    c\cdot \int_0^2 x^2 \ dx &= 1\\
    \left. c \frac{x^3}{3}\right\rvert_0^2 &= \\
    c\frac{8}{3} &=\\
    c &= \frac{3}{8}\\
    \therefore p(x) = \frac{3}{8}x^2
    \end{align}

Now that we have a PDF, the first step in inverse transform sampling is to calculate a Cumulative Distribution Function (CDF).  We solve for the CDF (:math:`P(x)`) as follows:

.. math::
    \begin{align}
    P(x) &= \int_0^x p(x) \ dx\\
    &=\int_0^x \frac{3}{8}x^2 \ dx\\
    &= \left. \frac{x^3}{8}\right\rvert_0^x\\
    \therefore P(x) &= \frac{x^3}{8}
    \end{align}

Now the final step in inverse transform sampling is to create an inverse function, such that we can feed it uniformly random samples :math:`zeta`, and obtain numbers which are distributed by our originally desired PDF.  To do this, we simply perform the following:

.. math::
    \begin{align}
    \zeta = P(x) &= \frac{x^3}{8}\\
    x = \left(8\zeta\right)^\frac{1}{3}
    \end{align}

Now, by uniformly sampling :math:`\zeta`, we can generate new sample points :math:`x` with a PDF of :math:`p(x) = \frac{3}{8}x^2`.  We can now use this to determine our new monte carlo estimator:

.. math::
    F \approx \frac{1}{N} \sum_{i=1}^N \frac{f(x_i)}{\frac{3}{8}x^2}

Where :math:`x` is no longer uniformly sampled, but rather is sampled from our PDF that more closely matches the shape of :math:`f(x)`.  If we for evaluate this for :math:`N=10` samples (10 times fewer than we did when uniformly sampling :math:`x`), we obtain an estimate of 4.4331.  Which is only about 1% off the true value!

Importance sampling requires more work, but can yield more accurate results with fewer samples, so long as you choose a sampling distribution that decently approximates the function you're integrating.  Look back at the integral in the rendering equation presented in :eq:`rendering-equation`:

.. math::
    \int_{\Omega} f_r(\vec{x},\omega_i,\omega_o,\lambda)L_i(\vec{x},\omega_o,\lambda)(\omega_i\cdot \hat{n}) \ d\omega_i

Consider what would happen if we had a small light source present in our scene.  If we only sampled rays by uniformly sampling the hemisphere above an intersection point :math:`\vec{x}`, we'd need a truly enormous number of rays to have any decent chance of sampling rays which intersect the light.  But we know the light is extremely important, as it is what contributes radiance to the scene!  Hence, we can apply this importance sampling method to directly sample the light (which will know will greatly contribute to the overall radiance we're calculating), while still accounting for the samples in a statistically rigorous way.


Multiple Importance Sampling
----------------------------

