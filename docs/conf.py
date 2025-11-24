# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration options, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import sys

# Get absolute path to this file
conf_dir = os.path.abspath(os.path.dirname(__file__))

# Use environment variable set by CMake if available
virapy_dir = os.environ.get('VIRAPY_BUILD_DIR')
if virapy_dir and os.path.exists(virapy_dir):
    sys.path.insert(0, virapy_dir)
    print(f"[conf.py] Added virapy path: {virapy_dir}")

project = 'Vira'
copyright = '2025, Chris Gnam'
author = 'Chris Gnam'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'breathe',
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.napoleon',
    'sphinx.ext.viewcode',
    'sphinx.ext.githubpages',
    'sphinx.ext.mathjax',
    'sphinx.ext.autosectionlabel',
    'sphinx_rtd_theme',
    'sphinx_autodoc_typehints',
    'sphinx_toolbox.sidebar_links',
    'myst_parser'
]

autodoc_default_options = {
    'members': True,
    'member-order': 'bysource',
    'special-members': '__init__',
    'undoc-members': True,
    'exclude-members': '__weakref__'
}

# Mock imports for any dependencies that might not be available during doc build
autodoc_mock_imports = []

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

myst_heading_anchors = 3
numfig = True
autosectionlabel_prefix_document = True

# Breathe Configuration
breathe_default_project = "vira"

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_static_path = ['_static']
html_logo = "_static/vira-wide.png"

# Add global toc tree to all html pages
html_sidebars = { '**': ['globaltoc.html', 'relations.html', 'sourcelink.html', 'searchbox.html'] }

# Custom RTD settings:
html_theme_options = {
    'collapse_navigation': False,
    'logo_only': True,
    'style_external_links': True
}

# Custom CSS overrides:
html_css_files = [
    'css/custom.css',
]