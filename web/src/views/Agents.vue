<template>
  <div class="agents">
    <div class="header">
      <h2>AI Agent</h2>
      <el-button type="primary" @click="openCreateDialog">
        <el-icon><Plus /></el-icon> 创建Agent
      </el-button>
    </div>

    <el-table :data="agents" v-loading="loading">
      <el-table-column prop="icon" label="图标" width="60">
        <template #default="{ row }">
          <el-icon :size="24"><component :is="row.icon || 'Robot'" /></el-icon>
        </template>
      </el-table-column>
      <el-table-column prop="name" label="名称" />
      <el-table-column prop="description" label="描述" show-overflow-tooltip />
      <el-table-column prop="model_name" label="模型" width="120" />
      <el-table-column label="工具" width="100">
        <template #default="{ row }">
          <el-tag v-if="row.tools_enabled?.length" size="small">
            {{ row.tools_enabled.length }}个
          </el-tag>
          <span v-else class="text-muted">无</span>
        </template>
      </el-table-column>
      <el-table-column prop="is_active" label="状态" width="80">
        <template #default="{ row }">
          <el-tag :type="row.is_active ? 'success' : 'info'" size="small">
            {{ row.is_active ? '启用' : '禁用' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="200">
        <template #default="{ row }">
          <el-button link type="primary" @click="testAgent(row)">对话</el-button>
          <el-button link type="primary" @click="editAgent(row)">编辑</el-button>
          <el-button link type="primary" @click="showBindDialog(row)">绑定</el-button>
          <el-button link type="danger" @click="deleteAgent(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 创建/编辑Agent对话框 -->
    <el-dialog v-model="showDialog" :title="isEdit ? '编辑Agent' : '创建Agent'" width="700" destroy-on-close>
      <el-tabs v-model="activeTab">
        <el-tab-pane label="基本信息" name="basic">
          <el-form :model="form" label-width="100px">
            <el-form-item label="名称" required>
              <el-input v-model="form.name" placeholder="Agent名称" />
            </el-form-item>
            <el-form-item label="描述">
              <el-input v-model="form.description" type="textarea" :rows="2" />
            </el-form-item>
            <el-form-item label="图标">
              <el-select v-model="form.icon" placeholder="选择图标">
                <el-option v-for="icon in icons" :key="icon" :label="icon" :value="icon">
                  <el-icon><component :is="icon" /></el-icon>
                  <span style="margin-left: 8px">{{ icon }}</span>
                </el-option>
              </el-select>
            </el-form-item>
            <el-form-item label="API配置" required>
              <el-select v-model="form.api_config_id" placeholder="选择API配置" style="width: 100%">
                <el-option v-for="c in apiConfigs" :key="c.id" :label="c.name" :value="c.id" />
              </el-select>
            </el-form-item>
            <el-form-item label="模型">
              <el-input v-model="form.model_name" placeholder="gpt-4, claude-3-opus" />
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="提示词" name="prompt">
          <el-form :model="form" label-width="100px">
            <el-form-item label="系统提示词">
              <el-input v-model="form.system_prompt" type="textarea" :rows="6"
                placeholder="定义Agent的角色、能力和行为规范" />
            </el-form-item>
            <el-form-item label="消息模板">
              <el-input v-model="form.prompt_template" type="textarea" :rows="4"
                placeholder="用户消息模板，可使用 {message} 变量" />
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="人设" name="persona">
          <el-form :model="form.persona" label-width="100px">
            <el-form-item label="名字">
              <el-input v-model="form.persona.name" placeholder="Agent的名字" />
            </el-form-item>
            <el-form-item label="角色">
              <el-input v-model="form.persona.role" placeholder="如：智能家居助手" />
            </el-form-item>
            <el-form-item label="风格">
              <el-select v-model="form.persona.style" placeholder="对话风格">
                <el-option label="专业" value="professional" />
                <el-option label="友好" value="friendly" />
                <el-option label="幽默" value="humorous" />
                <el-option label="简洁" value="concise" />
              </el-select>
            </el-form-item>
            <el-form-item label="特点">
              <el-input v-model="form.persona.traits" type="textarea" :rows="2"
                placeholder="描述Agent的性格特点" />
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="工具" name="tools">
          <el-form label-width="100px">
            <el-form-item label="启用工具">
              <el-checkbox-group v-model="form.tools_enabled">
                <el-checkbox v-for="tool in availableTools" :key="tool.name" :value="tool.name">
                  <div>
                    <strong>{{ tool.name }}</strong>
                    <div class="tool-desc">{{ tool.description }}</div>
                  </div>
                </el-checkbox>
              </el-checkbox-group>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="参数" name="params">
          <el-form :model="form" label-width="120px">
            <el-form-item label="温度">
              <el-slider v-model="form.temperature" :min="0" :max="2" :step="0.1" show-input />
            </el-form-item>
            <el-form-item label="最大Token">
              <el-input-number v-model="form.max_tokens" :min="100" :max="32000" :step="100" />
            </el-form-item>
            <el-form-item label="Top P">
              <el-slider v-model="form.top_p" :min="0" :max="1" :step="0.1" show-input />
            </el-form-item>
            <el-form-item label="上下文窗口">
              <el-input-number v-model="form.context_window" :min="1" :max="100" />
              <span class="form-tip">保留最近N条消息作为上下文</span>
            </el-form-item>
            <el-form-item label="启用记忆">
              <el-switch v-model="form.memory_enabled" />
              <span class="form-tip">长期记忆功能</span>
            </el-form-item>
            <el-form-item label="输出格式">
              <el-select v-model="form.output_format">
                <el-option label="文本" value="text" />
                <el-option label="JSON" value="json" />
                <el-option label="Markdown" value="markdown" />
              </el-select>
            </el-form-item>
          </el-form>
        </el-tab-pane>
      </el-tabs>

      <template #footer>
        <el-button @click="showDialog = false">取消</el-button>
        <el-button type="primary" @click="saveAgent">{{ isEdit ? '保存' : '创建' }}</el-button>
      </template>
    </el-dialog>

    <!-- 绑定设备对话框 -->
    <el-dialog v-model="bindDialog" title="绑定设备" width="400">
      <el-select v-model="bindDeviceId" placeholder="选择设备" style="width: 100%">
        <el-option v-for="d in devices" :key="d.id" :label="d.name" :value="d.id" />
      </el-select>
      <template #footer>
        <el-button @click="bindDialog = false">取消</el-button>
        <el-button type="primary" @click="bindDevice">绑定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'

const router = useRouter()
const agents = ref([])
const devices = ref([])
const apiConfigs = ref([])
const availableTools = ref([])
const loading = ref(false)
const showDialog = ref(false)
const bindDialog = ref(false)
const bindDeviceId = ref(null)
const currentAgent = ref(null)
const isEdit = ref(false)
const activeTab = ref('basic')

const icons = ['Robot', 'ChatDotRound', 'Service', 'Cpu', 'Monitor', 'Connection']

const defaultForm = () => ({
  name: '',
  description: '',
  icon: 'Robot',
  api_config_id: null,
  model_name: '',
  system_prompt: '',
  prompt_template: '',
  persona: { name: '', role: '', style: '', traits: '' },
  tools_enabled: [],
  temperature: 0.7,
  max_tokens: 2000,
  top_p: 1.0,
  context_window: 20,
  memory_enabled: false,
  output_format: 'text'
})

const form = reactive(defaultForm())

const loadAgents = async () => {
  loading.value = true
  try {
    const res = await api.agents.list()
    agents.value = res.data || []
  } finally {
    loading.value = false
  }
}

const loadTools = async () => {
  const res = await api.agents.tools()
  availableTools.value = res.data || []
}

const openCreateDialog = () => {
  isEdit.value = false
  Object.assign(form, defaultForm())
  activeTab.value = 'basic'
  showDialog.value = true
}

const editAgent = async (row) => {
  isEdit.value = true
  currentAgent.value = row
  const res = await api.agents.get(row.id)
  if (res.code === 0) {
    const data = res.data
    Object.assign(form, {
      name: data.name,
      description: data.description,
      icon: data.icon,
      api_config_id: data.api_config_id,
      model_name: data.model_name,
      system_prompt: data.system_prompt,
      prompt_template: data.prompt_template,
      persona: data.persona || { name: '', role: '', style: '', traits: '' },
      tools_enabled: data.tools_enabled || [],
      temperature: data.temperature,
      max_tokens: data.max_tokens,
      top_p: data.top_p,
      context_window: data.context_window,
      memory_enabled: data.memory_enabled,
      output_format: data.output_format
    })
    activeTab.value = 'basic'
    showDialog.value = true
  }
}

const saveAgent = async () => {
  if (!form.name) {
    ElMessage.warning('请输入名称')
    return
  }

  const data = { ...form }
  let res
  if (isEdit.value) {
    res = await api.agents.update(currentAgent.value.id, data)
  } else {
    res = await api.agents.create(data)
  }

  if (res.code === 0) {
    ElMessage.success(isEdit.value ? '保存成功' : '创建成功')
    showDialog.value = false
    loadAgents()
  }
}

const deleteAgent = async (row) => {
  await ElMessageBox.confirm('确定删除该Agent？相关对话记录也将被删除', '提示', { type: 'warning' })
  const res = await api.agents.delete(row.id)
  if (res.code === 0) {
    ElMessage.success('删除成功')
    loadAgents()
  }
}

const testAgent = (row) => {
  router.push(`/agents/${row.id}/chat`)
}

const loadDevices = async () => {
  const res = await api.devices.list()
  devices.value = res.data || []
}

const loadApiConfigs = async () => {
  const res = await api.gateway.list()
  apiConfigs.value = res.data || []
}

const showBindDialog = (row) => {
  currentAgent.value = row
  bindDeviceId.value = null
  bindDialog.value = true
}

const bindDevice = async () => {
  if (!bindDeviceId.value) {
    ElMessage.warning('请选择设备')
    return
  }
  const res = await api.agents.bind(currentAgent.value.id, bindDeviceId.value)
  if (res.code === 0) {
    ElMessage.success('绑定成功')
    bindDialog.value = false
  }
}

onMounted(() => {
  loadAgents()
  loadDevices()
  loadApiConfigs()
  loadTools()
})
</script>

<style scoped>
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}
.header h2 { margin: 0; }
.text-muted { color: #999; }
.tool-desc { font-size: 12px; color: #666; margin-top: 2px; }
.form-tip { margin-left: 10px; color: #999; font-size: 12px; }
</style>
