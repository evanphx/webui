enum js_value_type {
  js_value_null,
  js_value_bool,
  js_value_string,
  js_value_number
};

struct js_value {
  int type;
  union {
    int integer;
    char* ptr;
    double number;
  } v;
};
