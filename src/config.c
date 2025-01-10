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
    // widgets
    if (config->widgets && config->widget_count) {
        for (size_t i = 0; i < config->widget_count; i++) {
            widget_free(&config->widgets[i]);
        }
    }

    if(config->widgets) {
        free(config->widgets);
    }

    // fonts
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

    // pngs
    if (config->pngs && config->png_count) {
        for (size_t i = 0; i < config->png_count; i++) {
            if (config->pngs[i].filename) {
              free(config->pngs[i].filename);
            }

            if (config->pngs[i].data) {
              free(config->pngs[i].data);
            }
        }
    }

    if (config->pngs) {
        free(config->pngs);
    }

    if (config->ft) {
        FT_Done_FreeType(config->ft);
    }
}

int add_png(Config *config, struct json_object_s *png_obj) {
    int ret = -1;

    int width, height, color_type, bit_depth;
    struct json_object_element_s *elem = png_obj->start;
    const char* filename;
    FILE *fp = NULL;
    uint8_t *image_data = NULL;
    png_bytep *row_pointers = NULL;
    png_structp png_ptr = NULL;

    // walk through font object properties
    while (elem != NULL) {
        if (strcmp(elem->name->string, "filename") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            filename = value->string;
        }
        elem = elem->next;
    }

    if (!filename) {
        perror("Error: Could not load png file");
        goto cleanup;
    }

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: File %s could not be opened for reading\n", filename);
        goto cleanup;
    }

    // Read the PNG header
    uint8_t header[HEADER_SIZE];
    if (fread(header, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
        fprintf(stderr, "Error: Failed to read PNG header from file %s\n", filename);
        goto cleanup;
    }

    if (png_sig_cmp(header, 0, HEADER_SIZE)) {
        fprintf(stderr, "Error: File %s is not recognized as a PNG file\n", filename);
        goto cleanup;
    }

    // Initialize libpng structures
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "Error: png_create_read_struct failed\n");
        goto cleanup;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "Error: png_create_info_struct failed\n");
        goto cleanup;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error: An error occurred during PNG processing\n");
        goto cleanup;
    }

    // Set up PNG I/O
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, HEADER_SIZE);

    // Read PNG info
    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    // Validate dimensions
    if (width > MAX_WIDTH || height > MAX_HEIGHT) {
        fprintf(stderr, "Error: Image dimensions are too large (%dx%d)\n", width, height);
        goto cleanup;
    }

    // Transformations for consistent output
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    // Allocate memory for the image
    int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    if (row_bytes <= 0) {
        fprintf(stderr, "Error: Invalid row byte size\n");
        goto cleanup;
    }

    size_t total_size = row_bytes * (height);
    if (total_size > SIZE_MAX) {
        fprintf(stderr, "Error: Image size exceeds addressable memory\n");
        goto cleanup;
    }

    image_data = malloc(total_size);
    if (!image_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        goto cleanup;
    }

    // Allocate row pointers
    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * (height));
    if (!row_pointers) {
        fprintf(stderr, "Error: Memory allocation for row pointers failed\n");
        goto cleanup;
    }

    for (int i = 0; i < height; i++) {
        row_pointers[i] = image_data + i * row_bytes;
    }

    png_read_image(png_ptr, row_pointers);

    uint16_t *rgb565_image_data = malloc(width * height * sizeof(uint16_t));
    if (!rgb565_image_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        goto cleanup;
    }

    int bytes_per_pixel = (color_type == PNG_COLOR_TYPE_RGBA) ? 4 : 3;

    for (int i = 0; i < width * height; i++) {
        uint8_t r = image_data[i * bytes_per_pixel];
        uint8_t g = image_data[i * bytes_per_pixel + 1];
        uint8_t b = image_data[i * bytes_per_pixel + 2];

        // Convert to RGB565 and store
        rgb565_image_data[i] = rgb_to_rgb565(r, g, b);
    }

    free(image_data);

    // add png
    if (config->pngs == NULL) {
        config->png_count = 0;
        config->pngs = malloc(sizeof(Png));
        if (config->pngs == NULL) {
            perror("Error: Memory allocation failed");
            goto cleanup;
        }
    } else {
        Png *new_ptr = realloc(config->pngs, sizeof(Png) * (config->png_count + 1));
        if (new_ptr == NULL) {
            perror("Error: Memory allocation failed");
            goto cleanup;
        }
        config->pngs = new_ptr;
    }

    config->pngs[config->png_count].filename = calloc(1, strlen(filename) + 1);
    if (config->pngs[config->png_count].filename == NULL) {
        perror("Error: Memory allocation failed");
        goto cleanup;
    }
    memcpy(config->pngs[config->png_count].filename, filename, strlen(filename));

    config->pngs[config->png_count].data = rgb565_image_data;
    config->pngs[config->png_count].width = width;
    config->pngs[config->png_count].height = height;
    config->pngs[config->png_count].size = total_size;
    config->png_count++;
    ret = 0;
    goto done;

    cleanup:
    if (image_data) {
        free(image_data);
    }

    done:
    if (row_pointers) {
        free(row_pointers);
    }

    if (png_ptr) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    }

    if (fp) {
        fclose(fp);
    }

    return ret;
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
        perror("Error: Could not load font file");
        goto cleanup;
    }

    if (!ft_init(filename, &f, &config->ft, size)) {
        perror("Error: Could not load font file");
        goto cleanup;
    }

    // add font
    if (config->fonts == NULL) {
        config->font_count = 0;
        config->fonts = malloc(sizeof(Font));
        if (config->fonts == NULL) {
            perror("Error: Memory allocation failed");
            goto cleanup;
        }
    } else {
        Font *new_ptr = realloc(config->fonts, sizeof(Font) * (config->font_count + 1));
        if (new_ptr == NULL) {
            perror("Error: Memory allocation failed");
            goto cleanup;
        }
        config->fonts = new_ptr;
    }

    config->fonts[config->font_count].filename = calloc(1, strlen(filename) + 1);
    if (config->fonts[config->font_count].filename == NULL) {
        perror("Error: Memory allocation failed");
        goto cleanup;
    }
    memcpy(config->fonts[config->font_count].filename, filename, strlen(filename));

    config->fonts[config->font_count].face = f;
    config->font_count++;
    return 0;

    cleanup:
    if (f) {
        free(f);
    }
    return -1;
}

int add_widget(Config *config, struct json_object_s *widget_obj) {
    Widget *w = calloc(1, sizeof(Widget));
    w->scale = 5;
    if (w == NULL) {
        perror("Error: Memory allocation failed");
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
        else if (strcmp(elem->name->string, "precision") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t precision = strtol(value->number, NULL, 10);
            if (precision > 10) {
                precision = 10;
            }
            w->precision = precision;
        }
        else if (strcmp(elem->name->string, "scale") == 0) {
            struct json_number_s *value = json_value_as_number(elem->value);
            size_t scale = strtol(value->number, NULL, 10);
            if (scale == 0 || scale > 720) {
                scale = 1;
            }
            w->scale = scale;
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
                perror("Error: Memory allocation failed");
                free(w);
                return -1;
            }
            memcpy(w->type, value->string, strlen(value->string));

        }
        else if (strcmp(elem->name->string, "identifier") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->identifier = calloc(1, strlen(value->string) + 1);
            if (w->identifier == NULL) {
                perror("Error: Memory allocation failed");
                free(w);
                return -1;
            }
            memcpy(w->identifier, value->string, strlen(value->string));
        }
        else if (strcmp(elem->name->string, "text") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->text = calloc(1, strlen(value->string) + 1);
            if (w->text == NULL) {
                perror("Error: Memory allocation failed");
                free(w);
                return -1;
            }
            memcpy(w->text, value->string, strlen(value->string));
        }
        else if (strcmp(elem->name->string, "font") == 0 || strcmp(elem->name->string, "png") == 0) {
            struct json_string_s *value = json_value_as_string(elem->value);
            w->filename = calloc(1, strlen(value->string) + 1);
            if (w->filename == NULL) {
                perror("Error: Memory allocation failed");
                free(w);
                return -1;
            }
            memcpy(w->filename, value->string, strlen(value->string));
        }
        elem = elem->next;
    }

    // add widget
    if (config->widgets == NULL) {
        config->widget_count = 0;
        config->widgets = malloc(sizeof(Widget));
        if (config->widgets == NULL) {
            perror("Error: Memory allocation failed");
            free(w);
            return -1;
        }
    } else {
        Widget *new_ptr = realloc(config->widgets, sizeof(Widget) * (config->widget_count + 1));
        if (new_ptr == NULL) {
            perror("Error: Memory allocation failed");
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
        perror("Error: Failed to open config file");
        goto cleanup;
    }

    fseek(fd, 0, SEEK_END);
    long length = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    content = malloc(length + 1);
    if (!content) {
        perror("Error: Memory allocation failed");
        goto cleanup;
    }

    fread(content, 1, length, fd);
    content[length] = '\0';

    root = json_parse(content, length);
    if (!root || root->type != json_type_object) {
        perror("Error: Failed to parse JSON or JSON root is not an object\n");
        goto cleanup;
    }

    struct json_object_s *obj = json_value_as_object(root);
    struct json_object_element_s *elem = obj->start;
    while (elem != NULL) {
        if ((strcmp(elem->name->string, "widgets") == 0 || strcmp(elem->name->string, "fonts") == 0 || strcmp(elem->name->string, "pngs") == 0) && elem->value->type == json_type_array) {
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
                    } else if (strcmp(elem->name->string, "pngs") == 0) {
                        if (add_png(config, obj) != 0) {
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

    // go through widgets and link them to their fonts/pngs if appropriate
    for (size_t i = 0; i < config->widget_count; i++) {
        if (config->widgets[i].filename) {
            if (strcmp(config->widgets[i].type, "text") == 0 || strcmp(config->widgets[i].type, "value") == 0) {
                for(size_t j = 0; j < config->font_count; j++) {
                    if (config->fonts[j].filename && strcmp(config->widgets[i].filename, config->fonts[j].filename) == 0) {
                        config->widgets[i].face = config->fonts[j].face;
                    }
                }
            }
            else if (strcmp(config->widgets[i].type, "png") == 0) {
                for(size_t j = 0; j < config->png_count; j++) {
                    if (config->pngs[j].filename && strcmp(config->widgets[i].filename, config->pngs[j].filename) == 0) {
                        config->widgets[i].png = &config->pngs[j];
                    }
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