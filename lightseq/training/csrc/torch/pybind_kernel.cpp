#include <ATen/cuda/CUDAContext.h>
#include <cuda_fp16.h>
#include <torch/extension.h>

#include <cuda.h>

#include "cuda_util.h"
#include "kernels.h"

typedef const torch::Tensor cts;
typedef torch::Tensor ts;

template <typename T>
const T *rptr(const torch::Tensor &tensor) {
  return reinterpret_cast<const T *>(tensor.data_ptr());
}

template <typename T>
T *rptr(torch::Tensor &tensor) {
  return reinterpret_cast<T *>(tensor.data_ptr());
}

template <typename T>
void torch_launch_transform_0213(torch::Tensor &output,
                                 const torch::Tensor &vals, int batch_size,
                                 int seq_len, int hidden_dim, int nhead) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_transform_0213(rptr<T>(output), rptr<T>(vals), batch_size, seq_len,
                        hidden_dim, nhead, stream);
  //   cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_bias_add_transform_20314(torch::Tensor &output,
                                           const torch::Tensor &input,
                                           const torch::Tensor &bias, int dim_0,
                                           int dim_1, int dim_2, int dim_3,
                                           int dim_4) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_bias_add_transform_20314(rptr<T>(output), rptr<T>(input),
                                  rptr<T>(bias), dim_0, dim_1, dim_2, dim_3,
                                  dim_4, stream);
  //   cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_transform4d_0213(torch::Tensor &output,
                                   const torch::Tensor &vals, int batch_size,
                                   int seq_len, int hidden_dim, int nhead,
                                   int trans_count) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_transform4d_0213(rptr<T>(output), rptr<T>(vals), batch_size, seq_len,
                          hidden_dim, nhead, trans_count, stream);
  //   cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_attn_softmax(torch::Tensor &vals,
                               const torch::Tensor &attn_mask, int batch_size,
                               int nhead, int from_len, int to_len,
                               bool is_dec_self_attn, bool mask_future) {
  const T *attn_mask_ptr = rptr<T>(attn_mask);
  if (is_dec_self_attn) {
    attn_mask_ptr = nullptr;
  }
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_attn_softmax(rptr<T>(vals), attn_mask_ptr, batch_size, nhead, from_len,
                      to_len, mask_future, stream);
  //   cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_attn_softmax_bw(torch::Tensor &out_grad,
                                  const torch::Tensor &soft_inp, int rows,
                                  int softmax_len) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_attn_softmax_bw(rptr<T>(out_grad), rptr<T>(soft_inp), rows,
                         softmax_len, stream);
  //   cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_fused_add2(torch::Tensor &out, const torch::Tensor &inp1,
                             const torch::Tensor &inp2, int batch_size,
                             int seq_len, int hidden_dim) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_fused_add2(rptr<T>(out), rptr<T>(inp1), rptr<T>(inp2), batch_size,
                    seq_len, hidden_dim, stream);
  //     cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_ffn_bias_bwd(const torch::Tensor &inp, torch::Tensor &out,
                               int rows, int cols) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_fuse_transpose_bias_kernel(rptr<T>(inp), rptr<T>(out), rows, cols,
                                    stream);
  //     cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_layer_norm(torch::Tensor &ln_res, torch::Tensor &vars,
                             torch::Tensor &means, const torch::Tensor &inp,
                             const torch::Tensor &scale,
                             const torch::Tensor &bias, int batch_size,
                             int hidden_dim, bool with_mean) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  if (with_mean) {
    launch_layer_norm(rptr<T>(ln_res), rptr<T>(vars), rptr<T>(means),
                      rptr<T>(inp), rptr<T>(scale), rptr<T>(bias), batch_size,
                      hidden_dim, stream);
  } else {
    launch_layer_norm(rptr<T>(ln_res), rptr<T>(vars), (T *)nullptr,
                      rptr<T>(inp), rptr<T>(scale), rptr<T>(bias), batch_size,
                      hidden_dim, stream);
  }
}

template <typename T>
void torch_launch_ln_bw(torch::Tensor &gamma_grad, torch::Tensor &betta_grad,
                        torch::Tensor &inp_grad, const torch::Tensor &out_grad,
                        const torch::Tensor &residual_grad,
                        const torch::Tensor &inp_or_out,
                        const torch::Tensor &gamma, const torch::Tensor &betta,
                        const torch::Tensor &vars, const torch::Tensor &means,
                        int batch_size, int hidden_dim, bool with_mean,
                        bool fuse_add) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  cudaStream_t streams[2] = {stream, stream};
  const T *p_residual_grad;
  const T *p_betta;
  const T *p_means;

  if (fuse_add) {
    p_residual_grad = rptr<T>(residual_grad);
  } else {
    p_residual_grad = nullptr;
  }
  if (with_mean) {
    p_means = rptr<T>(means);
    p_betta = nullptr;
  } else {
    p_means = nullptr;
    p_betta = rptr<T>(betta);
  }

  launch_ln_bw(rptr<T>(gamma_grad), rptr<T>(betta_grad), rptr<T>(inp_grad),
               rptr<T>(out_grad), p_residual_grad, rptr<T>(inp_or_out),
               rptr<T>(gamma), p_betta, rptr<T>(vars), p_means, batch_size,
               hidden_dim, streams);
}

void torch_curand_init(int batch_size, int hidden_dim) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_curand_init(batch_size * hidden_dim, hidden_dim, stream);
}

template <typename T>
void torch_launch_concat3_dim1(const torch::Tensor &inp1,
                               const torch::Tensor &inp2, torch::Tensor &output,
                               int sz0, int sz2, int sz1_1, int sz1_2) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_concat3_dim1(rptr<T>(inp1), rptr<T>(inp2), rptr<T>(output), sz0, sz2,
                      sz1_1, sz1_2, stream);
  // cudaStreamSynchronize(stream);
  CHECK_GPU_ERROR(cudaGetLastError());
}

template <ActivationType actType, typename T>
void torch_launch_ls_dropout_act_bias(torch::Tensor &output,
                                      torch::Tensor &mask,
                                      const torch::Tensor &input,
                                      const torch::Tensor &bias, int total_seq,
                                      int hidden_dim, float ratio) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_ls_dropout_act_bias<actType, T>(
      rptr<T>(output), rptr<T>(input), rptr<uint8_t>(mask), rptr<T>(bias),
      total_seq * hidden_dim, hidden_dim, ratio, stream);
}

template <ActivationType actType, typename T>
void torch_launch_ls_dropout_act_bias_bwd(
    torch::Tensor &in_grad, torch::Tensor &bias_grad, torch::Tensor &mask,
    const torch::Tensor &input, const torch::Tensor &bias,
    const torch::Tensor &out_grad, int total_seq, int hidden_dim, float ratio) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  launch_ls_dropout_act_bias_bwd<actType, T>(
      rptr<T>(in_grad), rptr<T>(bias_grad), rptr<T>(input), rptr<T>(bias),
      rptr<T>(out_grad), rptr<uint8_t>(mask), total_seq, hidden_dim, ratio,
      stream);
}

void torch_launch_split_multilg_request(const torch::Tensor &req,
                                        torch::Tensor &src_lang_id,
                                        torch::Tensor &trg_lang_id,
                                        torch::Tensor &src_token_id) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  int batch_size = req.size(0);
  int req_len = req.size(1);
  launch_split_multilg_request(rptr<int>(req), rptr<int>(src_lang_id),
                               rptr<int>(trg_lang_id), rptr<int>(src_token_id),
                               batch_size, req_len, stream);
  // cudaStreamSynchronize(stream);
  // CHECK_GPU_ERROR(cudaGetLastError());
}

template <typename T>
void torch_launch_enc_emb(const torch::Tensor &token_emb,
                          const torch::Tensor &pos_emb,
                          const torch::Tensor &token_id,
                          const torch::Tensor &lang_emb,
                          const torch::Tensor &lang_id, torch::Tensor &res,
                          torch::Tensor &pad_mask, int pad_id,
                          int multilg_type) {
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  int batch_size = res.size(0);
  int seq_len = res.size(1);
  int hidden_dim = res.size(2);
  if (multilg_type == 0) {
    launch_enc_emb(rptr<T>(token_emb), rptr<T>(pos_emb), rptr<int>(token_id),
                   rptr<T>(res), rptr<int>(pad_mask), pad_id, batch_size,
                   seq_len, hidden_dim, stream);
    return;
  }
  if (multilg_type == 1) {
    launch_enc_emb_multilg_token(
        rptr<T>(token_emb), rptr<T>(pos_emb), rptr<int>(token_id),
        rptr<T>(lang_emb), rptr<int>(lang_id), rptr<T>(res),
        rptr<int>(pad_mask), pad_id, batch_size, seq_len, hidden_dim, stream);
    return;
  }
  if (multilg_type == 2) {
    launch_enc_emb_multilg_sentence(
        rptr<T>(token_emb), rptr<T>(pos_emb), rptr<int>(token_id),
        rptr<T>(lang_emb), rptr<int>(lang_id), rptr<T>(res),
        rptr<int>(pad_mask), pad_id, batch_size, seq_len, hidden_dim, stream);
    return;
  }
}
/*
template void ker_dec_embedding_launcher<float>(
    int step_token_num, int hidden_size, cudaStream_t stream,
    const float *token_emb, const float *pos_emb, const int *token_id,
    float *output, int step, int max_step, int vocab_size,
    int max_thread_per_block);
token_emb: [hidden_size, vocab_size], note, it is different with encoder
pos_emb: [max_step, hidden_size]
tokens: input token id, [batch_size, beam_size, max_step]
lang_emb: [lang_num, hidden_size]
lang_id: [batch_size, ]
res: result, [batch_size, beam_size, hidden_size]
*/
template <typename T>
void torch_launch_dec_emb(const torch::Tensor &token_emb,
                          const torch::Tensor &pos_emb,
                          const torch::Tensor &tokens,
                          const torch::Tensor &lang_emb,
                          const torch::Tensor &lang_id, torch::Tensor &res,
                          int step, int multilg_type) {
  int hidden_dim = token_emb.size(0);
  int vocab_size = token_emb.size(1);
  int max_step = pos_emb.size(0);
  int batch_size = tokens.size(0);
  int beam_size = tokens.size(1);
  cudaStream_t stream = at::cuda::getCurrentCUDAStream();
  ker_dec_embedding_launcher(batch_size * beam_size, hidden_dim, stream,
                             rptr<T>(token_emb), rptr<T>(pos_emb),
                             rptr<int>(tokens), rptr<T>(res), step, max_step,
                             vocab_size, 1024);
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.def("torch_launch_transform_0213_fp32", &torch_launch_transform_0213<float>,
        "Test kernel wrapper");
  m.def("torch_launch_transform_0213_fp16",
        &torch_launch_transform_0213<__half>, "Test kernel wrapper");
  m.def("torch_launch_bias_add_transform_20314_fp32",
        &torch_launch_bias_add_transform_20314<float>, "Test kernel wrapper");
  m.def("torch_launch_bias_add_transform_20314_fp16",
        &torch_launch_bias_add_transform_20314<__half>, "Test kernel wrapper");
  m.def("torch_launch_transform4d_0213_fp32",
        &torch_launch_transform4d_0213<float>, "Test kernel wrapper");
  m.def("torch_launch_transform4d_0213_fp16",
        &torch_launch_transform4d_0213<__half>, "Test kernel wrapper");
  m.def("torch_launch_fused_add2_fp32", &torch_launch_fused_add2<float>,
        "Test kernel wrapper");
  m.def("torch_launch_fused_add2_fp16", &torch_launch_fused_add2<__half>,
        "Test kernel wrapper");
  m.def("torch_launch_ffn_bias_bwd_fp32", &torch_launch_ffn_bias_bwd<float>,
        "Test kernel wrapper");
  m.def("torch_launch_ffn_bias_bwd_fp16", &torch_launch_ffn_bias_bwd<__half>,
        "Test kernel wrapper");
  m.def("torch_launch_attn_softmax_fp32", &torch_launch_attn_softmax<float>,
        "Test kernel wrapper");
  m.def("torch_launch_attn_softmax_fp16", &torch_launch_attn_softmax<__half>,
        "Test kernel wrapper");
  m.def("torch_launch_attn_softmax_bw_fp32",
        &torch_launch_attn_softmax_bw<float>, "Test kernel wrapper");
  m.def("torch_launch_attn_softmax_bw_fp16",
        &torch_launch_attn_softmax_bw<__half>, "Test kernel wrapper");
  m.def("torch_launch_layer_norm_fp32", &torch_launch_layer_norm<float>,
        "Test kernel wrapper");
  m.def("torch_launch_layer_norm_fp16", &torch_launch_layer_norm<__half>,
        "Test kernel wrapper");
  m.def("torch_launch_ln_bw_fp32", &torch_launch_ln_bw<float>,
        "Test kernel wrapper");
  m.def("torch_launch_ln_bw_fp16", &torch_launch_ln_bw<__half>,
        "Test kernel wrapper");
  m.def("torch_launch_curand_init", &torch_curand_init, "Test kernel wrapper");
  m.def("torch_launch_concat3_dim1_fp32", &torch_launch_concat3_dim1<float>,
        "Test kernel wrapper");
  m.def("torch_launch_concat3_dim1_fp16", &torch_launch_concat3_dim1<__half>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_relu_bias_fp32",
        &torch_launch_ls_dropout_act_bias<ActivationType::kRelu, float>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_relu_bias_fp16",
        &torch_launch_ls_dropout_act_bias<ActivationType::kRelu, __half>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_gelu_bias_fp32",
        &torch_launch_ls_dropout_act_bias<ActivationType::kGelu, float>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_gelu_bias_fp16",
        &torch_launch_ls_dropout_act_bias<ActivationType::kGelu, __half>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_relu_bias_bwd_fp32",
        &torch_launch_ls_dropout_act_bias_bwd<ActivationType::kRelu, float>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_relu_bias_bwd_fp16",
        &torch_launch_ls_dropout_act_bias_bwd<ActivationType::kRelu, __half>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_gelu_bias_bwd_fp32",
        &torch_launch_ls_dropout_act_bias_bwd<ActivationType::kGelu, float>,
        "Test kernel wrapper");
  m.def("torch_launch_ls_dropout_gelu_bias_bwd_fp16",
        &torch_launch_ls_dropout_act_bias_bwd<ActivationType::kGelu, __half>,
        "Test kernel wrapper");
  m.def("torch_launch_split_multilg_request",
        &torch_launch_split_multilg_request, "Test kernel wrapper");
  m.def("torch_launch_enc_emb_fp32", &torch_launch_enc_emb<float>,
        "Test kernel wrapper");
  m.def("torch_launch_enc_emb_fp16", &torch_launch_enc_emb<__half>,
        "Test kernel wrapper");
  m.def("torch_launch_dec_emb_fp32", &torch_launch_dec_emb<float>,
        "Test kernel wrapper");
  m.def("torch_launch_dec_emb_fp16", &torch_launch_dec_emb<__half>,
        "Test kernel wrapper");
}
