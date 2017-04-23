#include <Python.h>
#include <structmember.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pinyin.h>

typedef struct {
    PyObject_HEAD
    pinyin_instance_t *instance;
} SimplePinyin;

static pinyin_context_t *context;

static void
SimplePinyin_dealloc(SimplePinyin* self)
{
    pinyin_free_instance(self->instance);
    // printf("DEBUG: pinyin_free_instance\n");
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SimplePinyin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SimplePinyin *self;

    self = (SimplePinyin *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->instance = pinyin_alloc_instance(context);
        // printf("DEBUG: pinyin_alloc_instance\n");
        if (self->instance == NULL) {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *)self;
}

static PyObject *
SimplePinyin_convert(SimplePinyin* self, PyObject *args, PyObject *kwds)
{
    const char *pinyin = "";
    const char *prefix = "";
	static char *kwlist[] = {"pinyin", "prefix", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s", kwlist, &pinyin, &prefix))
	    return NULL;
	// printf("DEBUG: pinyin=%s, prefix=%s.\n", pinyin, prefix);	    
	pinyin_parse_more_full_pinyins(self->instance, pinyin);
    pinyin_guess_sentence_with_prefix(self->instance, prefix);
    pinyin_guess_full_pinyin_candidates(self->instance, 0);
    
    guint num = 0;
	guint16 *arr = NULL; //FIXME: Use a name better than `arr`
    pinyin_get_n_pinyin(self->instance, &num);
    arr = PyMem_New(guint16, num);
    // printf("DEBUG: num=%i, arr=%p.\n", num, arr);
    for (size_t i = 0; i < num; ++i) {
    	ChewingKeyRest *key_rest = NULL; 
        pinyin_get_pinyin_key_rest(self->instance, i, &key_rest);
        pinyin_get_pinyin_key_rest_length(self->instance, key_rest, &arr[i]);
        if (i > 0) {
            arr[i] += arr[i-1]; 
        }
        // printf("DEBUG: %i\n", arr[i]);
    }
    
    guint len = 0;
    pinyin_get_n_candidate(self->instance, &len);
    // printf("DEBUG: len=%i\n", len);
    PyObject *candidate_list = PyList_New(len);
    PyObject *match_len_list = PyList_New(len);
    for (size_t i = 0; i < len; ++i) {
        lookup_candidate_t * candidate = NULL;
        pinyin_get_candidate(self->instance, i, &candidate);

        const char * word = NULL;
        pinyin_get_candidate_string(self->instance, candidate, &word);
        PyObject *ob_word = NULL;
        ob_word = Py_BuildValue("s", word);
        PyList_SetItem(candidate_list, i, ob_word);
        
        lookup_candidate_type_t type;
        pinyin_get_candidate_type(self->instance, candidate, &type);
        // printf("DEBUG: type=%i\n", type);
        
        int cursor = pinyin_choose_candidate(self->instance, 0, candidate);
        int match_len = 0;
        int index = 0;
        switch (type) {
        case BEST_MATCH_CANDIDATE:
            match_len = strlen(pinyin);
            break;
        case DIVIDED_CANDIDATE:
        //FIXME: we assume that only one key get divided
            index = cursor-2;
            //FIXME: remove the below hack if possible
            if (index >= num) {
                index = num-1;
            }
            match_len = arr[index];
            break;
        case RESPLIT_CANDIDATE: 
        case NORMAL_CANDIDATE:
            index = cursor-1;
            match_len = arr[index];
        default:
            break;
        }
        
        // printf("DEBUG: match_len=%i\n", match_len);
        PyObject *ob_match_len = NULL;
        ob_match_len = Py_BuildValue("i", match_len);
        PyList_SetItem(match_len_list, i, ob_match_len);    
        
        pinyin_clear_constraint(self->instance, 0);
        // printf("DEBUG: %s %d\n", word, arr[cursor-1]);
    }
    
    PyMem_Del(arr);
    pinyin_reset(self->instance);
    
    PyObject *ob_pair = NULL;
    ob_pair = Py_BuildValue("(O,O)", candidate_list, match_len_list);
    
    return ob_pair;
}

static PyMethodDef SimplePinyin_methods[] = {
    {"convert", (PyCFunction)SimplePinyin_convert, METH_VARARGS | METH_KEYWORDS,
     NULL
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject SimplePinyinType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "simplepinyin.SimplePinyin",             /* tp_name */
    sizeof(SimplePinyin),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)SimplePinyin_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,   /* tp_flags */
    "SimplePinyin objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SimplePinyin_methods,             /* tp_methods */
    0,
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,
    0,                         /* tp_alloc */
    SimplePinyin_new,                 /* tp_new */
};

static PyModuleDef simplepinyinmodule = {
    PyModuleDef_HEAD_INIT,
    "simplepinyin",
    NULL,
    -1,
    NULL, NULL, NULL, NULL, NULL
};

static void
libpinyin_cleanup(void)
{
    pinyin_fini(context);
    // printf("DEBUG: pinyin_fini\n");
}

PyMODINIT_FUNC
PyInit_simplepinyin(void) 
{
    context = pinyin_init(LIBPINYIN_DATA, "/tmp");
    // printf("DEBUG: pinyin_init\n");
    if (context == NULL)
        return NULL;
    
    pinyin_option_t options =
        PINYIN_CORRECT_ALL | USE_DIVIDED_TABLE | USE_RESPLIT_TABLE |
        0;
    if (!pinyin_set_options(context, options))
        return NULL;
        
    if (Py_AtExit(libpinyin_cleanup) == -1)
        return NULL;

    PyObject* m;

    if (PyType_Ready(&SimplePinyinType) < 0)
        return NULL;

    m = PyModule_Create(&simplepinyinmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&SimplePinyinType);
    PyModule_AddObject(m, "SimplePinyin", (PyObject *)&SimplePinyinType);
    return m;
}
