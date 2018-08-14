import sphinx.writers.html
import sphinx.ext.mathjax

class HTMLTranslator(sphinx.writers.html.HTMLTranslator):
    def visit_displaymath(self, node):
        sphinx.ext.mathjax.html_visit_displaymath(self, node)
    def depart_displaymath(self, node):
        return
    def visit_math(self, node):
        sphinx.ext.mathjax.html_visit_math(self, node)
    def depart_math(self, node):
        return
    def depart_document(self, node):
        methods_by_class = {}
        for i, tag in enumerate(self.body):
            if tag.strip() == '<dl class="method">':
                method_id = self.body[i + 1].split('"')[1].split('.')
                method_name = method_id[-1]
                method_class = '.'.join(method_id[: -1])
                if method_class not in methods_by_class:
                    methods_by_class[method_class] = []
                methods_by_class[method_class].append(method_name)
        class_names = set(methods_by_class.keys())
        for i, tag in enumerate(self.body):
            if tag.strip() == '<dl class="class">':
                class_names.add(self.body[i + 1].split('"')[1])
        functions = []
        for i, tag in enumerate(self.body):
            if tag.strip() == '<dl class="function">':
                try:
                    functions.append(self.body[i + 1].split('"')[1])
                except:
                    functions.append(self.body[i + 3])
        for i, tag in enumerate(self.body):
            if tag.strip() == '<dl class="data">':
                data_id = self.body[i + 1].split('"')[1].split('.')
                data_name = data_id[-1]
                data_class = '.'.join(data_id[: -1])
                if data_class:
                    if data_class not in methods_by_class:
                        methods_by_class[data_class] = []
                    methods_by_class[data_class].append(data_name)
                else:
                    functions.append(data_name)
        
        # sort everything
        sort_key = lambda s: s.lower()
        functions = sorted(functions, key=sort_key)
        class_names = sorted(list(class_names), key=sort_key)
        for key, values in zip(methods_by_class.keys(), methods_by_class.values()):
            methods_by_class[key] = sorted(values, key=sort_key)
        
        # add any missing class names to methods_by_class
        for name in class_names:
            if name not in methods_by_class: methods_by_class[name] = []
        
        jump_table = []
        if functions:
            fn_jmps = ['<a href="#%s" title="Link to this definition">%s</a>' % (name, name) for name in functions]
            jump_table += ['<p>', ' &middot; '.join(fn_jmps), '</p>']
        
        for cl in class_names:
            method_jmps = ['<a href="#%s.%s" title="Link to this definition">%s</a>' % (cl, name, name) for name in methods_by_class[cl]]
            jump_table += ['<p>', '<dl class="docutils"><dt><a href="#%s" title="Link to this definition">%s</a></dt><dd>%s</dd></dl>' % (cl, cl, ' &middot; '.join(method_jmps)), '</p>']
            
        self.body = jump_table + self.body
        sphinx.writers.html.HTMLTranslator.depart_document(self, node)
