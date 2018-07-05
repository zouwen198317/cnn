#include "normalize_layer.h"

normalize_layer *make_normalize_layer(int w, int h, int c, int batch)
{
	int inputs = w * h * c;
    fprintf(stderr, "Normalize:          %d x %d x %d, %d inputs\n", w, h, c, inputs);
    normalize_layer *l = calloc(1, sizeof(normalize_layer));
    l->w = w;
    l->h = h;
    l->c = c;
    l->inputs = inputs;
    l->batch = batch;
    l->output = calloc(inputs*batch, sizeof(float));
    l->delta = calloc(inputs*batch, sizeof(float));
#ifdef GPU
    l->output_gpu = cuda_make_array(l->output, inputs*batch);
    l->delta_gpu = cuda_make_array(l->delta, inputs*batch);
#endif
    return l;
} 

void resize_normalize_layer(const normalize_layer *l, int inputs)
{
}

void forward_normalize_layer(const normalize_layer *l, float *input)
{
    memcpy(l->output, input, l->inputs * l->batch * sizeof(float));
    l2normalize_cpu(l->output, l->batch, l->c, l->w*l->h);
}

void backward_normalize_layer(const normalize_layer *l, float *delta)
{
    memcpy(delta, l->delta, l->inputs * l->batch * sizeof(float));
}

#ifdef GPU

void forward_normalize_layer_gpu(const normalize_layer *l, float *input)
{
    cuda_mem_copy(l->output_gpu, input, l->inputs*l->batch);
    l2normalize_gpu(l->output_gpu, l->batch, l->c, l->w*l->h);
    /*char cuda_compare_error_string[128] = {0};
    sprintf(cuda_compare_error_string, "\n%s", "forward_normalize_layer_gpu output");
    cuda_compare(l->output_gpu, l->output, l->inputs*l->batch, cuda_compare_error_string);*/
}

void backward_normalize_layer_gpu(const normalize_layer *l, float *delta)
{
    cuda_mem_copy(delta, l->delta_gpu, l->inputs*l->batch);
}

#endif