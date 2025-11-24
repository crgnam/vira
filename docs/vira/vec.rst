Vec
===============================================
For the time being, Vira uses a set of of `vec` and `mat` objects which are simply typdef'd to GLM vec/mat types.


.. doxygentypedef:: vira::vec1

.. doxygentypedef:: vira::vec2

.. doxygentypedef:: vira::vec3

.. doxygentypedef:: vira::vec4

.. doxygentypedef:: vira::mat2

.. doxygentypedef:: vira::mat3

.. doxygentypedef:: vira::mat4

.. doxygentypedef:: vira::mat23

.. doxygentypedef:: vira::Pixel

.. doxygentypedef:: vira::UV

.. doxygentypedef:: Normal


Vec Equality Operators
----------------------

.. doxygenfunction:: vira::operator<(const T& vec1, const T& vec2)

.. doxygenfunction:: vira::operator>(const T& vec1, const T& vec2)

.. doxygenfunction:: vira::operator<=(const T& vec1, const T& vec2)

.. doxygenfunction:: vira::operator>=(const T& vec1, const T& vec2)

.. doxygenfunction:: vira::operator==(const T& vec1, const T& vec2)

.. doxygenstruct:: vira::vec2LessCompare
    

Additional Operator Overloads
-----------------------------

.. doxygenfunction:: vira::operator*(const T& vec, const N& numeric)

.. doxygenfunction:: vira::operator*(const N& numeric, const T& vec)


Vector operations
-----------------
.. doxygenfunction:: vira::normalize

.. doxygenfunction:: vira::dot

.. doxygenfunction:: vira::length

.. doxygenfunction:: vira::cross

.. doxygenfunction:: vira::transpose

.. doxygenfunction:: vira::validVec


Vec Constraints
---------------
.. doxygenconcept:: vira::IsFloatVec

.. doxygenconcept:: vira::IsDoubleVec

.. doxygenconcept:: vira::IsFloatingVec

.. doxygenconcept:: vira::IsVec

.. doxygenconcept:: vira::IsUV

.. doxygenconcept:: vira::IsFloatVec3

.. doxygenconcept:: vira::IsDoubleVec3

.. doxygenconcept:: vira::IsMat