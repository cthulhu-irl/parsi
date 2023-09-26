import os
from glob import glob

# -- Project information -------------------------------------------------

project = 'parsi'
copyright = '2023, Mohsen Mirkarimi'
author = 'Mohsen Mirkarimi'

# -- General configuration -----------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.

extensions = ['breathe', 'm2r2']
source_suffix = ['.rst', '.md']

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

primary_domain = 'cpp'
highlight_language = 'cpp'

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
# html_static_path = ['_static']

# Breathe Configuration
breathe_default_project = project

project_dir = os.path.abspath(os.path.join(os.getcwd(), ".."))
header_files = glob(os.path.join(f"{project_dir}/include/**/*.hpp"), recursive=True)
breathe_projects_source = {
    project: ('include', list(map(lambda path: path.removeprefix(f"{project_dir}/include"), header_files)))
}

# -- Generate API References -------------------------------------------------

base_path = "api"
if not os.path.exists(base_path):
    os.mkdir(base_path)

with open(f"{base_path}/index.rst", "w") as index_file:
    index_file.write("API References\n")
    index_file.write("==============\n\n")
    index_file.write(".. toctree::\n")
    index_file.write("   :maxdepth: 1\n\n")

    for header_filepath in header_files:
        filepath = header_filepath.removeprefix(f"{project_dir}/include\\")
        header_file_rst_name = filepath.removesuffix(".hpp").replace("\\", "__")
        header_file_doxy_path = filepath.replace("\\", "/")

        index_file.write(f"   {header_file_rst_name}\n")

        with open(f"{base_path}/{header_file_rst_name}.rst", "w") as header_rst_file:
            title = header_file_doxy_path
            header_rst_file.write(f"{title}\n{'=' * len(title)}\n\n")
            header_rst_file.write(f".. doxygenfile:: {header_file_doxy_path}\n")
            header_rst_file.write(f"   :project: {project}\n\n")
