# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= poetry run sphinx-build
SPHINXAPIDOC  ?= poetry run sphinx-apidoc
PYTHON_SRC	  ?= modfs
SOURCEDIR     = doc
BUILDDIR      = doc/build

EXCLUDE_PRIVATE	= _source,_asdict,_fields,_field_defaults,_field_types,_replace,_make
APIDOC_OPTIONS	='members,private-members,undoc-members,show-inheritance,inherited-members,change-1'

ifeq ($(OS),Windows_NT)
	RM = pwsh -c Remove-Item -Recurse -Force -ErrorAction Ignore
else
	RM = rm -rf
endif


# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile clean docs

docs:
	$(RM) $(SOURCEDIR)/api || true
	SPHINX_APIDOC_OPTIONS=$(APIDOC_OPTIONS) $(SPHINXAPIDOC) -M -o $(SOURCEDIR)/api/ $(PYTHON_SRC)
	sed -i.bak 's/change-1:/exclude-members: $(EXCLUDE_PRIVATE)/' $(SOURCEDIR)/api/*.rst
	$(MAKE) html

clean:
	$(RM) $(BUILDDIR) || true

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
