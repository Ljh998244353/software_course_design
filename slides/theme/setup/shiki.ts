import theme from './themes/one-purple-unicorn.json'
import { cudaTransformer } from './cuda-transformer'

export default async () => ({
  theme: theme as any,
  transformers: [
    cudaTransformer(),
  ],
})
