from setuptools import setup
from setuptools.dist import Distribution

class BinaryDistribution(Distribution):
    """Distribution which always forces a binary package with platform name"""
    def has_ext_modules(self):
        return True

# All metadata comes from pyproject.toml
setup(
    distclass=BinaryDistribution,
)