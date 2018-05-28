#include "network.h"

struct network *make_network(int n)
{
    struct network *net = calloc(1, sizeof(struct network));
    net->n = n;
    net->layers = calloc(net->n, sizeof(void *));
    net->layers_type = calloc(net->n, sizeof(enum LAYER_TYPE));
    net->batch = 1;
    net->seen = 0;
    net->batch_train = 0;
    net->correct_num = 0;
    net->correct_num_count = 0;
    return net;
}

float get_current_learning_rate(struct network * net)
{
    switch (net->policy) {
        case STEPS:
            for(int i = 0; i < net->num_steps; ++i){
                if(net->steps[i] == net->batch_train){
                	net->learning_rate *= net->scales[i];
                }
            }
            return net->learning_rate;;
        default:
            //fprintf(stderr, "Policy is weird!\n");
            return net->learning_rate;
    }
}

void forward_network(struct network *net, float *input)
{
    int i;
    for(i = 0; i < net->n; ++i){
        if(net->layers_type[i] == CONVOLUTIONAL){
            convolutional_layer *layer = (convolutional_layer *)net->layers[i];
            forward_convolutional_layer(layer, input);
            input = layer->output;
        }else if(net->layers_type[i] == CONNECTED){
            connected_layer *layer = (connected_layer *)net->layers[i];
            forward_connected_layer(layer, input);
            input = layer->output;
        } else if(net->layers_type[i] == MAXPOOL){
            maxpool_layer *layer = (maxpool_layer *)net->layers[i];
            forward_maxpool_layer(layer, input);
            input = layer->output;

        } else if(net->layers_type[i] == AVGPOOL){
            avgpool_layer *layer = (avgpool_layer *)net->layers[i];
            forward_avgpool_layer(layer, input);
            input = layer->output;
        } else if(net->layers_type[i] == SOFTMAX){
            softmax_layer *layer = (softmax_layer *)net->layers[i];
            forward_softmax_layer(layer, input);
            input = layer->output;
        } else if(net->layers_type[i] == COST){
            cost_layer *layer = (cost_layer *)net->layers[i];
            forward_cost_layer(layer, input, net);
            input = layer->output;
        } else {
            printf("forward_network layers_type error, layer: %d\n", i);
            exit(-1);
        }
    }
}

void update_network(struct network *net, float step)
{
    int i;
    for(i = 0; i < net->n; ++i){
        if(net->layers_type[i] == CONVOLUTIONAL){
            convolutional_layer *layer = (convolutional_layer *)net->layers[i];
            update_convolutional_layer(layer, net->learning_rate, net->momentum, net->decay);
        } else if(net->layers_type[i] == CONNECTED){
            connected_layer *layer = (connected_layer *)net->layers[i];
            update_connected_layer(layer, net->learning_rate, net->momentum, net->decay);
        } else if(net->layers_type[i] == MAXPOOL){
        } else if(net->layers_type[i] == AVGPOOL){
        } else if(net->layers_type[i] == SOFTMAX){
        } else if(net->layers_type[i] == COST){
        } else {
            printf("get_network_layer_data layers_type error, layer: %d\n", i);
            exit(-1);
        }
    }
}

/* data_type: 0: output, 1: delta */
float *get_network_layer_data(struct network *net, int i, int data_type)
{
    if(net->layers_type[i] == CONVOLUTIONAL){
        convolutional_layer *layer = (convolutional_layer *)net->layers[i];
        return data_type == 0 ? layer->output : layer->delta;
    } else if(net->layers_type[i] == CONNECTED){
        connected_layer *layer = (connected_layer *)net->layers[i];
        return data_type == 0 ? layer->output : layer->delta;
    } else if(net->layers_type[i] == MAXPOOL){
        maxpool_layer *layer = (maxpool_layer *)net->layers[i];
        return data_type == 0 ? layer->output : layer->delta;
    } else if(net->layers_type[i] == AVGPOOL){
        avgpool_layer *layer = (avgpool_layer *)net->layers[i];
        return data_type == 0 ? layer->output : layer->delta;
    } else if(net->layers_type[i] == SOFTMAX){
        softmax_layer *layer = (softmax_layer *)net->layers[i];
        return data_type == 0 ? layer->output : layer->delta;
    } else if(net->layers_type[i] == COST){
        cost_layer *layer = (cost_layer *)net->layers[i];
        return data_type == 0 ? layer->output : layer->delta;
    } else {
        printf("get_network_layer_data layers_type error, layer: %d\n", i);
        exit(-1);
    }
}

float *get_network_output(struct network *net)
{
    return get_network_layer_data(net, net->n-1, 0);
}

void backward_network(struct network *net, float *input)
{
    float *prev_input;
    float *prev_delta;
    for(int i = net->n-1; i >= 0; --i){
        if(i == 0){
            prev_input = input;
            prev_delta = 0;
        }else{
            prev_input = get_network_layer_data(net, i-1, 0);
            prev_delta = get_network_layer_data(net, i-1, 1);
        }
        if(net->layers_type[i] == CONVOLUTIONAL){
            convolutional_layer *layer = (convolutional_layer *)net->layers[i];
            learn_convolutional_layer(layer, prev_input);
            if(i != 0) backward_convolutional_layer(layer, prev_delta);
        } else if(net->layers_type[i] == CONNECTED){
            connected_layer *layer = (connected_layer *)net->layers[i];
            learn_connected_layer(layer, prev_input);
            if(i != 0) backward_connected_layer(layer, prev_delta);
        } else if(net->layers_type[i] == MAXPOOL){
            maxpool_layer *layer = (maxpool_layer *)net->layers[i];
            if(i != 0) backward_maxpool_layer(layer, prev_input, prev_delta);
        } else if(net->layers_type[i] == AVGPOOL){
            avgpool_layer *layer = (avgpool_layer *)net->layers[i];
            if(i != 0) backward_avgpool_layer(layer, prev_delta);
        } else if(net->layers_type[i] == SOFTMAX){
            softmax_layer *layer = (softmax_layer *)net->layers[i];
            if(i != 0) backward_softmax_layer(layer, prev_delta);
        } else if(net->layers_type[i] == COST){
            cost_layer *layer = (cost_layer *)net->layers[i];
            backward_cost_layer(layer, prev_delta);
        } else {
            printf("get_network_layer_data layers_type error, layer: %d\n", i);
            exit(-1);
        }

    }
}

void train_network_batch(struct network *net, batch b)
{
    for(int i = 0; i < b.n; ++i){
        //show_image(b.images[i], "Input");
        net->truth = b.truth[i];
        forward_network(net, b.images[i].data);
        backward_network(net, b.images[i].data);
        update_network(net, .001);
    }
    net->seen += net->batch;
    net->correct_num_count += net->batch;
    if(net->correct_num_count > 1000){
    	net->correct_num_count = 0;
    	net->correct_num = 0;
    }
    net->batch_train += 1;
}

int get_network_output_size_layer(struct network *net, int i)
{
    if(net->layers_type[i] == CONVOLUTIONAL){
        convolutional_layer *layer = (convolutional_layer *)net->layers[i];
        image output = get_convolutional_image(layer);
        return output.h*output.w*output.c;
    } else if(net->layers_type[i] == CONNECTED){
        connected_layer *layer = (connected_layer *)net->layers[i];
        return layer->outputs;
    } else if(net->layers_type[i] == MAXPOOL){
        maxpool_layer *layer = (maxpool_layer *)net->layers[i];
        image output = get_maxpool_image(layer);
        return output.h*output.w*output.c;
    }else if(net->layers_type[i] == AVGPOOL){
        avgpool_layer *layer = (avgpool_layer *)net->layers[i];
        return layer->outputs;
    }else if(net->layers_type[i] == SOFTMAX){
        softmax_layer *layer = (softmax_layer *)net->layers[i];
        return layer->inputs;
    }else if(net->layers_type[i] == COST){
        cost_layer *layer = (cost_layer *)net->layers[i];
        return layer->outputs;
    } else {
        printf("get_network_output_size_layer layers_type error, layer: %d\n", i);
        exit(-1);
    }
}

int get_network_output_size(struct network *net)
{
    int i = net->n-1;
    return get_network_output_size_layer(net, i);
}

image get_network_image_layer(struct network *net, int i)
{
    if(net->layers_type[i] == CONVOLUTIONAL){
        convolutional_layer *layer = (convolutional_layer *)net->layers[i];
        return get_convolutional_image(layer);
    }
    else if(net->layers_type[i] == MAXPOOL){
        maxpool_layer *layer = (maxpool_layer *)net->layers[i];
        return get_maxpool_image(layer);
    } else {
        printf("get_network_image_layer layers_type error, layer: %d\n", i);
        exit(-1);
    }
}

void save_weights(struct network *net, char *filename)
{
#ifdef GPU
    if(net->gpu_index >= 0){
        cuda_set_device(net->gpu_index);
    }
#endif
    fprintf(stderr, "Saving weights to %s\n", filename);
    FILE *fp = fopen(filename, "wb");
    if(!fp) file_error(filename);

    int major = 0;
    int minor = 2;
    int revision = 0;
    printf("\nweights version info: major: %d, minor: %d, revision: %d\n", major, minor, revision);

    fwrite(&major, sizeof(int), 1, fp);
    fwrite(&minor, sizeof(int), 1, fp);
    fwrite(&revision, sizeof(int), 1, fp);
    fwrite(net->seen, sizeof(size_t), 1, fp);

    for(int i = 0; i < net->n; ++i){
        layer l = net->layers[i];
        if (l.dontsave) continue;
        if(l.type == CONVOLUTIONAL || l.type == DECONVOLUTIONAL){
            save_convolutional_weights(l, fp);
        } if(l.type == CONNECTED){
            save_connected_weights(l, fp);
        }
    }
    fclose(fp);
}

void load_weights(struct network *net, char *filename)
{
#ifdef GPU
    if(net->gpu_index >= 0){
        cuda_set_device(net->gpu_index);
    }
#endif
    fprintf(stderr, "Loading weights from %s\n", filename);
    fflush(stdout);
    FILE *fp = fopen(filename, "rb");
    if(!fp) file_error(filename);

    int major;
    int minor;
    int revision;
    fread(&major, sizeof(int), 1, fp);
    fread(&minor, sizeof(int), 1, fp);
    fread(&revision, sizeof(int), 1, fp);
    fread(net->seen, sizeof(int), 1, fp);
    printf("\nweights version info: major: %d, minor: %d, revision: %d\n", major, minor, revision);

    for(int i = 0; i < net->n; ++i){
        layer l = net->layers[i];
        if(l.type == CONVOLUTIONAL || l.type == DECONVOLUTIONAL){
            load_convolutional_weights(l, fp);
        }
        if(l.type == CONNECTED){
            load_connected_weights(l, fp, transpose);
        }
    }
    fprintf(stderr, "Loading weights Done!\n");
    fclose(fp);
}
