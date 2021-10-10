# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------

project = "Poor man's query language"
copyright = '2021, Anton Ksenzhuk'
author = 'Anton Ksenzhuk'


# -- General configuration ---------------------------------------------------


master_doc = 'index'
templates_path = []
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

extensions = [
    "breathe"
]

breathe_default_project = "pmql"


# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'

html_static_path = []
