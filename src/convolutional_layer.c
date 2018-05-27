#include "convolutional_layer.h"
#include "utils.h"
#include <stdio.h>

image get_convolutional_image(const convolutional_layer *layer)
{
    int h,w,c;
    h = (layer->h-1)/layer->stride + 1;
    w = (layer->w-1)/layer->stride + 1;

    c = layer->n;
    return float_to_image(h,w,c,layer->output);
}

image get_convolutional_delta(const convolutional_layer *layer)
{
    int h,w,c;
    h = (layer->h-1)/layer->stride + 1;
    w = (layer->w-1)/layer->stride + 1;

    c = layer->n;
    return float_to_image(h,w,c,layer->delta);
}

convolutional_layer *make_convolutional_layer(int h, int w, int c, int n, int size, int stride, ACTIVATION activation)
{
    int i;
    int out_h,out_w;
    convolutional_layer *layer = calloc(1, sizeof(convolutional_layer));
    layer->h = h;
    layer->w = w;
    layer->c = c;
    layer->n = n;
    layer->stride = stride;
    layer->kernels = calloc(n, sizeof(image));
    layer->kernel_updates = calloc(n, sizeof(image));
    layer->kernel_momentum = calloc(n, sizeof(image));
    layer->biases = calloc(n, sizeof(float));
    layer->bias_updates = calloc(n, sizeof(float));
    layer->bias_momentum = calloc(n, sizeof(float));
    float scale = 1./(size*size*c);
    for(i = 0; i < n; ++i){
        layer->kernels[i] = make_random_kernel(size, c, scale);
        layer->kernel_updates[i] = make_random_kernel(size, c, 0);
        layer->kernel_momentum[i] = make_random_kernel(size, c, 0);
    }
	out_h = (layer->h-1)/layer->stride + 1;
	out_w = (layer->w-1)/layer->stride + 1;


    fprintf(stderr, "Convolutional:      %d x %d x %d image, %d filters size: %d -> %d x %d x %d image\n",
            h,w,c, n,size, out_h, out_w, n);
    layer->output = calloc(out_h * out_w * n, sizeof(float));
    layer->delta  = calloc(out_h * out_w * n, sizeof(float));
    layer->activation = activation;

    return layer;
}

void forward_convolutional_layer(const convolutional_layer *layer, float *in)
{
    image input = float_to_image(layer->h, layer->w, layer->c, in);
    image output = get_convolutional_image(layer);
    int i,j;
    for(i = 0; i < layer->n; ++i){
        convolve(input, layer->kernels[i], layer->stride, i, output);
    }
    float max = 0.0F;
    float min = 0.0F;

    for(i = 0; i < output.c; ++i){
        for(j = 0; j < output.h*output.w; ++j){
            int index = i*output.h*output.w + j;
            output.data[index] += layer->biases[i];
            output.data[index] = activate(output.data[index], layer->activation);
            if(output.data[index] > max) max = output.data[index];
            if(output.data[index] < min) min = output.data[index];

        }
    }
    printf("forward_convolutional_layer %f %f\n", max, min);
}

void backward_convolutional_layer(const convolutional_layer *layer, float *delta)
{
    int i;

    image in_delta = float_to_image(layer->h, layer->w, layer->c, delta);
    image out_delta = get_convolutional_delta(layer);
    zero_image(in_delta);

    for(i = 0; i < layer->n; ++i){
        back_convolve(in_delta, layer->kernels[i], layer->stride, i, out_delta);
    }
}

void learn_convolutional_layer(const convolutional_layer *layer, float *input)
{
    image in_image = float_to_image(layer->h, layer->w, layer->c, input);
    image out_delta = get_convolutional_delta(layer);
    image out_image = get_convolutional_image(layer);
    for(int i = 0; i < out_image.h*out_image.w*out_image.c; ++i){
        out_delta.data[i] *= gradient(out_image.data[i], layer->activation);
    }
    for(int i = 0; i < layer->n; ++i){
        kernel_update(in_image, layer->kernel_updates[i], layer->stride, i, out_delta);
        layer->bias_updates[i] += avg_image_layer(out_delta, i);
    }
}

void update_convolutional_layer(const convolutional_layer *layer, float learning_rate, float momentum, float decay)
{
    int i,j;
    for(i = 0; i < layer->n; ++i){
        layer->bias_momentum[i] = learning_rate*(layer->bias_updates[i]) + momentum*layer->bias_momentum[i];
        layer->biases[i] += layer->bias_momentum[i];
        layer->bias_updates[i] = 0;
        int pixels = layer->kernels[i].h*layer->kernels[i].w*layer->kernels[i].c;
        for(j = 0; j < pixels; ++j){
            layer->kernel_momentum[i].data[j] = learning_rate*(layer->kernel_updates[i].data[j] - decay*layer->kernels[i].data[j])
                                                + momentum*layer->kernel_momentum[i].data[j];
            layer->kernels[i].data[j] += layer->kernel_momentum[i].data[j];
            //layer->kernels[i].data[j] = constrain(layer->kernels[i].data[j], 1.);
        }
        zero_image(layer->kernel_updates[i]);
    }
}