#
# THIS FILE IS GENERATED AND SHALL NOT BE EDITED.
#

# cmake-format: off
set(CODE_GENERATOR_JINJA_FILES
{% for template in templates | sort %}
    ${PROJECT_SOURCE_DIR}/src/language/templates/{{ template }}
{% endfor %}
)

set(CODE_GENERATOR_PY_FILES
{% for file in py_files | sort %}
    ${PROJECT_SOURCE_DIR}/src/language/{{ file }}
{% endfor %}
)

set(CODE_GENERATOR_YAML_FILES
{% for file in yaml_files | sort %}
    ${PROJECT_SOURCE_DIR}/src/language/{{ file }}
{% endfor %}
)

{% for dir, files in outputs | dictsort %}
set({{ dir | upper }}_GENERATED_SOURCES
  {% for file in files | sort %}
    ${PROJECT_BINARY_DIR}/src/{{ dir }}/{{ file }}
  {% endfor %}
)

{% endfor %}
set(NMODL_GENERATED_SOURCES
{% for dir in outputs.keys() | sort %}
    ${{ '{' }}{{ dir | upper }}_GENERATED_SOURCES}
{% endfor %}
)
# cmake-format: on
