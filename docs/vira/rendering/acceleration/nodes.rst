Acceleration Structure Nodes
===============================================

The purpose of these nodes are to allow the ray tracer to support transformations to the geometry without actually transforming the geometry itself.  When building the Top Level Acceleration Structure (TLAS), model transformations are inserted as nodes into the tree.  When ray tracing, the incoming rays are transformed by these nodes, allowing the geometry to be rendered as if it has been transformed, without actually changing the underlying geometry.

.. doxygenstruct:: vira::rendering::TreeNode
    :members:
    :undoc-members:

.. doxygenstruct:: vira::rendering::TLASNode
    :members:
    :undoc-members:

.. doxygenclass:: vira::rendering::TLASLeaf
    :members:
    :undoc-members: