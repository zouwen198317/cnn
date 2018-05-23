#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cost_layer.h"
#include "utils.h"

enum COST_TYPE get_cost_type(char *s)
{
    if (strcmp(s, "sse")==0) return SSE;
    if (strcmp(s, "masked")==0) return MASKED;
    if (strcmp(s, "smooth")==0) return SMOOTH;
    fprintf(stderr, "Couldn't find cost type %s, going with SSE\n", s);
    return SSE;
}

char *get_cost_string(enum COST_TYPE a)
{
    switch(a){
        case SSE:
            return "sse";
        case MASKED:
            return "masked";
        case SMOOTH:
            return "smooth";
        default:
            return "sse";
    }
}

cost_layer *make_cost_layer(int batch, int inputs, enum COST_TYPE cost_type, float scale)
{
    fprintf(stderr, "Cost:               %d inputs\n", inputs);
    cost_layer *l = calloc(1, sizeof(cost_layer));;
    l->scale = scale;
    l->batch = batch;
    l->inputs = inputs;
    l->outputs = inputs;
    l->cost_type = cost_type;
    l->delta = calloc(inputs*batch, sizeof(float));
    l->output = calloc(inputs*batch, sizeof(float));
    l->cost = calloc(1, sizeof(float));
    #ifdef GPU
    l->delta_gpu = cuda_make_array(l->output, inputs*batch);
    l->output_gpu = cuda_make_array(l->delta, inputs*batch);
    #endif
    return l;
}

void resize_cost_layer(cost_layer *l, int inputs)
{
    l->inputs = inputs;
    l->outputs = inputs;
    l->delta = realloc(l->delta, inputs*l->batch*sizeof(float));
    l->output = realloc(l->output, inputs*l->batch*sizeof(float));
#ifdef GPU
    cuda_free(l->delta_gpu);
    cuda_free(l->output_gpu);
    l->delta_gpu = cuda_make_array(l->delta, inputs*l->batch);
    l->output_gpu = cuda_make_array(l->output, inputs*l->batch);
#endif
}

void forward_cost_layer(const cost_layer *l, float *input, struct network *net)
{
    if (net->test) return;
    if(l->cost_type == MASKED){
        int i;
        for(i = 0; i < l->batch*l->inputs; ++i){
            if(net.truth[i] == SECRET_NUM) input[i] = SECRET_NUM;
        }
    }
    if(l->cost_type == SMOOTH){
        smooth_l1_cpu(l->batch*l->inputs, input, net.truth, l->delta, l->output);
    } else {
        l2_cpu(l->batch*l->inputs, input, net.truth, l->delta, l->output);
    }
    l->cost[0] = sum_array(l->output, l->batch*l->inputs);
}

void backward_cost_layer(const cost_layer *l, network_state state)
{
    axpy_cpu(l->batch*l->inputs, l->scale, l->delta, 1, state.delta, 1);
}

#ifdef GPU

void pull_cost_layer(cost_layer *l)
{
    cuda_pull_array(l->delta_gpu, l->delta, l->batch*l->inputs);
}

void push_cost_layer(cost_layer *l)
{
    cuda_push_array(l->delta_gpu, l->delta, l->batch*l->inputs);
}

void forward_cost_layer_gpu(cost_layer *l, network_state state)
{
    if (!state.truth) return;
    if (l->cost_type == MASKED) {
        mask_ongpu(l->batch*l->inputs, state.input, SECRET_NUM, state.truth);
    }

    if(l->cost_type == SMOOTH){
        smooth_l1_gpu(l->batch*l->inputs, state.input, state.truth, l->delta_gpu, l->output_gpu);
    } else {
        l2_gpu(l->batch*l->inputs, state.input, state.truth, l->delta_gpu, l->output_gpu);
    }

    cuda_pull_array(l->output_gpu, l->output, l->batch*l->inputs);
    l->cost[0] = sum_array(l->output, l->batch*l->inputs);
}

void backward_cost_layer_gpu(const cost_layer *l, network_state state)
{
    axpy_ongpu(l->batch*l->inputs, l->scale, l->delta_gpu, 1, state.delta, 1);
}
#endif

