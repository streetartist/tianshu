<template>
  <div class="gateway">
    <div class="header">
      <h2>API网关</h2>
      <el-button type="primary" @click="openAddDialog">
        <el-icon><Plus /></el-icon> 添加配置
      </el-button>
    </div>

    <el-table :data="configs" v-loading="loading">
      <el-table-column prop="name" label="名称" width="150" />
      <el-table-column prop="description" label="简介" />
      <el-table-column prop="api_type" label="类型" width="120">
        <template #default="{ row }">
          <el-tag>{{ typeMap[row.api_type] || row.api_type }}</el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="base_url" label="地址" />
      <el-table-column prop="total_calls" label="调用次数" width="100" />
      <el-table-column label="操作" width="150">
        <template #default="{ row }">
          <el-button link type="primary" @click="editConfig(row)">编辑</el-button>
          <el-button link type="danger" @click="deleteConfig(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 添加/编辑配置对话框 -->
    <el-dialog v-model="showDialog" :title="editingId ? '编辑配置' : '添加配置'" width="700">
      <el-tabs v-model="activeTab">
        <el-tab-pane label="基本信息" name="basic">
          <el-form :model="form" label-width="100px">
            <el-form-item label="名称">
              <el-input v-model="form.name" />
            </el-form-item>
            <el-form-item label="简介">
              <el-input v-model="form.description" placeholder="简要描述此API的用途" />
            </el-form-item>
            <el-form-item label="类型">
              <el-select v-model="form.api_type" style="width: 100%">
                <el-option label="AI模型" value="ai_model" />
                <el-option label="普通API" value="normal_api" />
                <el-option label="Python处理器" value="python_handler" />
              </el-select>
            </el-form-item>
            <el-form-item label="API端点" v-if="form.api_type === 'ai_model'">
              <el-input v-model="form.base_url" placeholder="https://api.deepseek.com/v1/chat/completions" />
            </el-form-item>
            <el-form-item label="API Key" v-if="form.api_type === 'ai_model'">
              <el-input v-model="form.api_key" type="password" show-password />
            </el-form-item>
            <el-form-item label="超时时间">
              <el-input-number v-model="form.timeout" :min="5" :max="300" /> 秒
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="请求配置" name="request" v-if="form.api_type === 'normal_api'">
          <el-form :model="form" label-width="100px">
            <el-form-item label="请求地址">
              <el-input v-model="form.base_url" placeholder="https://api.qaq.qa/api/hitokoto" />
              <div class="form-tip">完整的API地址</div>
            </el-form-item>
            <el-form-item label="请求方法">
              <el-select v-model="form.request_method" style="width: 150px">
                <el-option label="GET" value="GET" />
                <el-option label="POST" value="POST" />
              </el-select>
            </el-form-item>
            <el-form-item label="调用模式">
              <el-radio-group v-model="form.proxy_mode">
                <el-radio value="server">服务器中转</el-radio>
                <el-radio value="direct">设备直连</el-radio>
              </el-radio-group>
            </el-form-item>
            <el-form-item label="请求体" v-if="form.request_method === 'POST'">
              <el-input v-model="form.request_body_template" type="textarea" :rows="4"
                placeholder='{"key": "{{value}}"}' />
              <div class="form-tip">POST请求的JSON体，使用 {{参数名}} 作为变量</div>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="响应处理" name="response" v-if="form.api_type === 'normal_api'">
          <el-form :model="form" label-width="100px">
            <el-form-item label="响应提取">
              <el-input v-model="form.response_extract" placeholder="$.choices[0].message.content" />
              <div class="form-tip">JSONPath格式，如 $.data.result 或 $.choices[0].message.content</div>
            </el-form-item>
            <el-form-item label="处理代码" v-if="form.proxy_mode === 'server'">
              <el-input v-model="form.process_code" type="textarea" :rows="8"
                placeholder="# 可用变量: params(请求参数), response(原始响应)&#10;# 将结果赋值给 result&#10;result = response" />
              <div class="form-tip">Python代码，仅服务器中转模式可用</div>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="参数配置" name="params" v-if="form.api_type === 'python_handler'">
          <el-form :model="form" label-width="100px">
            <el-form-item label="参数模板">
              <el-input v-model="form.request_body_template" type="textarea" :rows="6"
                placeholder='{"message": "用户消息", "location": "城市代码"}' />
              <div class="form-tip">JSON格式，说明设备需要传入的参数。设备可通过API获取此模板了解参数格式。</div>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="处理器代码" name="handler" v-if="form.api_type === 'python_handler'">
          <el-form :model="form" label-width="100px">
            <el-form-item label="Python代码">
              <el-input v-model="form.handler_code" type="textarea" :rows="15"
                placeholder="# 完整的Python处理器代码&#10;# 可用变量: params (设备传入的参数字典)&#10;# 将返回结果赋值给 result&#10;# 可以导入任何已安装的库&#10;&#10;import json&#10;import datetime&#10;&#10;# 示例: 处理设备请求&#10;message = params.get('message', '')&#10;result = {&#10;    'reply': f'收到消息: {message}',&#10;    'time': datetime.datetime.now().isoformat()&#10;}" />
            </el-form-item>
            <div class="form-tip">
              完整的Python代码，可以导入任何已安装的库。设备调用时传入的参数通过 params 变量获取，将返回结果赋值给 result 变量。
            </div>
          </el-form>
        </el-tab-pane>
      </el-tabs>
      <template #footer>
        <el-button @click="showDialog = false">取消</el-button>
        <el-button type="primary" @click="saveConfig">{{ editingId ? '保存' : '创建' }}</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'

const configs = ref([])
const loading = ref(false)
const showDialog = ref(false)
const activeTab = ref('basic')
const editingId = ref(null)

const typeMap = {
  ai_model: 'AI模型',
  normal_api: '普通API',
  python_handler: 'Python处理器'
}

const form = reactive({
  name: '',
  description: '',
  api_type: 'ai_model',
  base_url: '',
  api_key: '',
  proxy_mode: 'server',
  request_method: 'GET',
  request_body_template: '',
  response_extract: '',
  process_code: '',
  handler_code: '',
  timeout: 30
})

const loadConfigs = async () => {
  loading.value = true
  try {
    const res = await api.gateway.list()
    configs.value = res.data || []
  } finally {
    loading.value = false
  }
}

const createConfig = async () => {
  const res = await api.gateway.create(form)
  if (res.code === 0) {
    ElMessage.success('配置创建成功')
    showDialog.value = false
    loadConfigs()
  }
}

const resetForm = () => {
  Object.assign(form, {
    name: '',
    description: '',
    api_type: 'ai_model',
    base_url: '',
    api_key: '',
    proxy_mode: 'server',
    request_method: 'GET',
    request_body_template: '',
    response_extract: '',
    process_code: '',
    handler_code: '',
    timeout: 30
  })
}

const openAddDialog = () => {
  editingId.value = null
  resetForm()
  activeTab.value = 'basic'
  showDialog.value = true
}

const editConfig = async (row) => {
  editingId.value = row.id
  const res = await api.gateway.get(row.id)
  if (res.code === 0) {
    Object.assign(form, res.data)
    activeTab.value = 'basic'
    showDialog.value = true
  }
}

const deleteConfig = async (row) => {
  try {
    await ElMessageBox.confirm('确定删除该配置？', '提示', { type: 'warning' })
    const res = await api.gateway.delete(row.id)
    if (res.code === 0) {
      ElMessage.success('删除成功')
      loadConfigs()
    }
  } catch (e) {}
}

const saveConfig = async () => {
  let res
  if (editingId.value) {
    res = await api.gateway.update(editingId.value, form)
  } else {
    res = await api.gateway.create(form)
  }
  if (res.code === 0) {
    ElMessage.success(editingId.value ? '保存成功' : '创建成功')
    showDialog.value = false
    loadConfigs()
  }
}

onMounted(loadConfigs)
</script>

<style scoped>
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}
.header h2 { margin: 0; }
.form-tip {
  font-size: 12px;
  color: #909399;
  margin-top: 4px;
}
</style>
