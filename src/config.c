#include "config.h"

uint16_t hex_to_rgb565(const char *hex_string) {
    if (hex_string == NULL) {
        return false;
    }

    if (hex_string[0] != '#' || strlen(hex_string) != 7) {
        return false;
    }

    for (int i = 1; i < 7; i++) {
        if (!isxdigit(hex_string[i])) {
            return false;
        }
    }

    unsigned int r, g, b;
    if (sscanf(hex_string + 1, "%2x%2x%2x", &r, &g, &b) != 3) {
        return false;
    }

    return rgb_to_rgb565(r, g, b);
}

void unload_config(Config *config) {
    if (config->widgets && config->widget_count) {
        for (size_t i = 0; i < config->widget_count; i++) {
            widget_free(&config->widgets[i]);
        }
    }

    if(config->widgets) {
        free(config->widgets);
    }

    if (config->fonts && config->font_count) {
        for (size_t i = 0; i < config->font_count; i++) {
            if (config->fonts[i].filename) {
              free(config->fonts[i].filename);
            }
        }
    }

    if (config->fonts) {
        free(config->fonts);
    }

    if (config->ft) {
        FT_Done_FreeType(config->ft);
    }

    printf("unloaded\n");
}

int add_font(Config *config, struct json_object_s *font_obj) {
    FT_Face f = NULL;
    struct json_object_element_s *elem = font_obj->start;
    const char* filename;
    size_t size = 0;

    // walk through font object properties
    while (elem != NULL) {
        if (strcmp(elem->name->string, "filename") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            filename = value->string;
        }
        else if (strcmp(elem->name->string, "size") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size = strtol(value->number, NULL, 10);
        }
        elem = elem->next;
    }

    if (!filename || !size) {
        perror("Could not load font file");
        return -1;
    }

    if (!ft_init(filename, &f, &config->ft, size)) {
        perror("Could not load font file");
        goto cleanup;
    }

    // add font
    if (config->fonts == NULL) {
        config->font_count = 0;
        config->fonts = malloc(sizeof(Font));
        if (config->fonts == NULL) {
            perror("Memory allocation error");
            goto cleanup;
        }
    } else {
        Font *new_ptr = realloc(config->fonts, sizeof(Font) * (config->font_count + 1));
        if (new_ptr == NULL) {
            perror("Memory allocation error");
            goto cleanup;
        }
        config->fonts = new_ptr;
    }

    config->fonts[config->font_count].filename = calloc(1, strlen(filename) + 1);
    if (config->fonts[config->font_count].filename == NULL) {
        perror("Memory allocation error");
        free(f);
        return -1;
    }
    memcpy(config->fonts[config->font_count].filename, filename, strlen(filename));

    config->fonts[config->font_count].face = f;
    config->font_count++;
    return 0;

    cleanup:
    free(f);
    return -1;
}

int add_widget(Config *config, struct json_object_s *widget_obj) {
    Widget *w = calloc(1, sizeof(Widget));
    if (w == NULL) {
        perror("Memory allocation error");
        return -1;
    }

    struct json_object_element_s *elem = widget_obj->start;
    // walk through widget object properties
    while (elem != NULL) {
        if (strcmp(elem->name->string, "top") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t top = strtol(value->number, NULL, 10);
            w->top = top;
        }
        else if (strcmp(elem->name->string, "left") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t left = strtol(value->number, NULL, 10);
            w->left = left;
        }
        else if (strcmp(elem->name->string, "width") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t width = strtol(value->number, NULL, 10);
            w->width = width;
        }
        else if (strcmp(elem->name->string, "height") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t height = strtol(value->number, NULL, 10);
            w->height = height;
        }
        else if (strcmp(elem->name->string, "min") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t min = strtol(value->number, NULL, 10);
            w->min = min;
        }
        else if (strcmp(elem->name->string, "max") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t max = strtol(value->number, NULL, 10);
            w->max = max;
        }
        else if (strcmp(elem->name->string, "border_color") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->border_color = hex_to_rgb565(value->string);
            w->has_border = true;
        }
        else if (strcmp(elem->name->string, "line_color") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->line_color = hex_to_rgb565(value->string);
        }
        else if (strcmp(elem->name->string, "type") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->type = calloc(1, strlen(value->string) + 1);
            if (w->type == NULL) {
                perror("Memory allocation error");
                free(w);
                return -1;
            }
            memcpy(w->type, value->string, strlen(value->string));

        }
        else if (strcmp(elem->name->string, "identifier") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->identifier = calloc(1, strlen(value->string) + 1);
            if (w->identifier == NULL) {
                perror("Memory allocation error");
                free(w);
                return -1;
            }
            memcpy(w->identifier, value->string, strlen(value->string));
        }
        else if (strcmp(elem->name->string, "text") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->text = calloc(1, strlen(value->string) + 1);
            if (w->text == NULL) {
                perror("Memory allocation error");
                free(w);
                return -1;
            }
            memcpy(w->text, value->string, strlen(value->string));
        }
        else if (strcmp(elem->name->string, "font") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->font = calloc(1, strlen(value->string) + 1);
            if (w->font == NULL) {
                perror("Memory allocation error");
                free(w);
                return -1;
            }
            memcpy(w->font, value->string, strlen(value->string));
        }
        elem = elem->next;
    }

    // add widget
    if (config->widgets == NULL) {
        config->widget_count = 0;
        config->widgets = malloc(sizeof(Widget));
        if (config->widgets == NULL) {
            perror("Memory allocation error");
            free(w);
            return -1;
        }
    } else {
        Widget *new_ptr = realloc(config->widgets, sizeof(Widget) * (config->widget_count + 1));
        if (new_ptr == NULL) {
            perror("Memory allocation error");
            free(w);
            return -1;
        }
        config->widgets = new_ptr;
    }

    memcpy(&(config->widgets)[config->widget_count], w, sizeof(Widget));
    config->widget_count++;
    free(w);

    return 0;
}

int load_config(const char *filename, Config *config) {
    int ret = -1;
    struct json_value_s *root = NULL;
    char *content = NULL;
    FILE *fd = NULL;

    // Open and read the JSON file
    fd = fopen(filename, "r");
    if (!fd) {
        perror("Failed to open config file");
        goto cleanup;
    }

    fseek(fd, 0, SEEK_END);
    long length = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    content = malloc(length + 1);
    if (!content) {
        perror("Memory allocation error");
        goto cleanup;
    }

    fread(content, 1, length, fd);
    content[length] = '\0';

    root = json_parse(content, length);
    if (!root || root->type != json_type_object) {
        perror("Failed to parse JSON or JSON root is not an object\n");
        goto cleanup;
    }

    struct json_object_s *obj = json_value_as_object(root);
    struct json_object_element_s *elem = obj->start;
    while (elem != NULL) {
        if ((strcmp(elem->name->string, "widgets") == 0 || strcmp(elem->name->string, "fonts") == 0) && elem->value->type == json_type_array) {
            struct json_array_s* array = json_value_as_array(elem->value);
            struct json_array_element_s* array_elem = array->start;
            // walk through array of widgets
            while(array_elem != NULL) {
                // each widget object
                if (array_elem->value->type == json_type_object) {
                    obj = json_value_as_object(array_elem->value);
                    if (strcmp(elem->name->string, "fonts") == 0) {
                        if (add_font(config, obj) != 0) {
                            goto cleanup;
                        }
                    } else if (strcmp(elem->name->string, "widgets") == 0) {
                        if (add_widget(config, obj) != 0) {
                            goto cleanup;
                        }
                    }
                }
                array_elem = array_elem->next;
            }
        }
        elem = elem->next;
    }

    // go through widgets and link them to their font if appropriate
    for (size_t i = 0; i < config->widget_count; i++) {
        if (config->widgets[i].font) {
            for(size_t j = 0; j < config->font_count; j++) {
                if (config->fonts[j].filename && strcmp(config->widgets[i].font, config->fonts[j].filename) == 0) {
                    config->widgets[i].face = config->fonts[j].face;
                }
            }
        }
    }

    ret = 0;

    cleanup:
    if (fd) {
        fclose(fd);
    }

    if (root) {
        free(root);
    }

    if (content) {
        free(content);
    }

    return ret;
}