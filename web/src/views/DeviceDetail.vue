<template>
  <div class="device-detail" v-loading="loading">
    <div class="header">
      <el-page-header @back="$router.back()">
        <template #content>{{ device.name }}</template>
      </el-page-header>
    </div>

    <el-tabs v-model="activeTab">
      <!-- 基本信息 -->
      <el-tab-pane label="基本信息" name="info">
        <el-descriptions :column="2" border>
          <el-descriptions-item label="设备ID">{{ device.device_id }}</el-descriptions-item>
          <el-descriptions-item label="类型">{{ template?.display_name || device.device_type }}</el-descriptions-item>
          <el-descriptions-item label="状态">
            <el-tag :type="device.status === 'online' ? 'success' : 'info'">
              {{ device.status === 'online' ? '在线' : '离线' }}
            </el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="最后在线">{{ device.last_seen || '-' }}</el-descriptions-item>
          <el-descriptions-item label="描述" :span="2">{{ device.description || '-' }}</el-descriptions-item>
        </el-descriptions>
      </el-tab-pane>

      <!-- 数据监控 -->
      <el-tab-pane label="数据监控" name="data">
        <div class="data-header">
          <span>最后更新: {{ lastUpdate || '-' }}</span>
          <el-button size="small" @click="loadDeviceData">刷新</el-button>
        </div>
        <el-row :gutter="16" class="data-cards">
          <el-col :span="6" v-for="dp in dataPoints" :key="dp.name">
            <el-card shadow="hover" class="data-card">
              <div class="data-label">{{ dp.label || dp.name }}</div>
              <div class="data-value">
                <span v-if="dp.type === 'bool'">
                  <el-tag :type="deviceData[dp.name] ? 'success' : 'info'">
                    {{ deviceData[dp.name] ? '是' : '否' }}
                  </el-tag>
                </span>
                <span v-else>{{ deviceData[dp.name] ?? '-' }}</span>
                <span class="data-unit">{{ dp.unit }}</span>
              </div>
            </el-card>
          </el-col>
        </el-row>
        <el-empty v-if="!dataPoints.length" description="该设备类型未定义数据点" />
      </el-tab-pane>

      <!-- 设备凭证 -->
      <el-tab-pane label="设备凭证" name="secret">
        <el-alert type="warning" :closable="false" style="margin-bottom: 16px">
          设备凭证用于硬件设备连接服务器，请妥善保管，不要泄露给他人。
        </el-alert>
        <el-descriptions :column="1" border>
          <el-descriptions-item label="Device ID">
            <code>{{ device.device_id }}</code>
            <el-button link size="small" @click="copyText(device.device_id)">复制</el-button>
          </el-descriptions-item>
          <el-descriptions-item label="Secret Key">
            <template v-if="secretKey">
              <code>{{ secretKey }}</code>
              <el-button link size="small" @click="copyText(secretKey)">复制</el-button>
            </template>
            <el-button v-else size="small" @click="loadSecret">点击查看</el-button>
          </el-descriptions-item>
        </el-descriptions>
        <div style="margin-top: 16px">
          <el-button type="danger" size="small" @click="resetSecret">重置密钥</el-button>
          <span class="form-tip">重置后旧密钥将失效，设备需要更新配置</span>
        </div>
      </el-tab-pane>

      <!-- 配置管理 -->
      <el-tab-pane label="配置管理" name="config">
        <div class="config-actions">
          <el-button type="primary" size="small" @click="showAddConfig = true">
            <el-icon><Plus /></el-icon> 添加配置
          </el-button>
        </div>
        <el-table :data="configList" style="margin-top: 10px">
          <el-table-column label="配置项" width="180">
            <template #default="{ row }">
              {{ row.label || row.key }}
              <span v-if="row.label" style="color: #999; font-size: 12px;"> ({{ row.key }})</span>
            </template>
          </el-table-column>
          <el-table-column prop="value" label="值" />
          <el-table-column label="操作" width="100">
            <template #default="{ row }">
              <el-button link type="primary" @click="editConfig(row)">编辑</el-button>
            </template>
          </el-table-column>
        </el-table>
      </el-tab-pane>
    </el-tabs>

    <!-- 添加配置对话框 -->
    <el-dialog v-model="showAddConfig" title="添加配置" width="400">
      <el-form :model="configForm">
        <el-form-item label="配置项">
          <el-input v-model="configForm.key" />
        </el-form-item>
        <el-form-item label="值">
          <el-input v-model="configForm.value" type="textarea" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showAddConfig = false">取消</el-button>
        <el-button type="primary" @click="saveConfig">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'

const route = useRoute()
const deviceId = route.params.id

const loading = ref(false)
const activeTab = ref('info')
const device = ref({})
const template = ref(null)
const dataPoints = ref([])
const deviceData = ref({})
const lastUpdate = ref('')
const configList = ref([])
const showAddConfig = ref(false)
const configForm = reactive({ key: '', value: '' })
const secretKey = ref('')

const loadDevice = async () => {
  loading.value = true
  try {
    const res = await api.devices.get(deviceId)
    device.value = res.data || {}
    // 加载设备类型模板
    if (device.value.device_type) {
      loadTemplate(device.value.device_type)
    }
  } finally {
    loading.value = false
  }
}

const loadTemplate = async (typeName) => {
  const res = await api.templates.list()
  const templates = res.data || []
  template.value = templates.find(t => t.name === typeName)
  dataPoints.value = template.value?.data_points || []
  loadDeviceData()
  loadConfig()
}

const loadDeviceData = async () => {
  const res = await api.data.get(deviceId, { limit: 1 })
  const records = res.data || []
  if (records.length > 0) {
    deviceData.value = records[0].data || {}
    lastUpdate.value = records[0].timestamp
  }
}

const loadConfig = async () => {
  const res = await api.devices.getConfig(deviceId)
  const data = res.data || {}

  // 构建标签映射（从模板获取）
  const labelMap = {
    heartbeat_interval: '心跳间隔(秒)',
    data_interval: '数据间隔(秒)',
    offline_timeout: '离线超时(秒)',
    protocol: '通信协议'
  }

  // 从模板的config_items获取自定义配置项标签
  if (template.value?.config_items) {
    template.value.config_items.forEach(item => {
      if (item.key && item.label) {
        labelMap[item.key] = item.label
      }
    })
  }

  configList.value = Object.entries(data).map(([key, value]) => ({
    key,
    value,
    label: labelMap[key] || ''
  }))
}

const saveConfig = async () => {
  await api.devices.updateConfig(deviceId, { [configForm.key]: configForm.value })
  ElMessage.success('配置已保存')
  showAddConfig.value = false
  loadConfig()
}

const editConfig = (row) => {
  configForm.key = row.key
  configForm.value = typeof row.value === 'object' ? JSON.stringify(row.value) : row.value
  showAddConfig.value = true
}

const loadSecret = async () => {
  const res = await api.devices.getSecret(deviceId)
  if (res.code === 0) {
    secretKey.value = res.data.secret_key
  }
}

const resetSecret = async () => {
  await ElMessageBox.confirm('重置后旧密钥将失效，设备需要更新配置，确定重置？', '警告', { type: 'warning' })
  const res = await api.devices.resetSecret(deviceId)
  if (res.code === 0) {
    secretKey.value = res.data.secret_key
    ElMessage.success('密钥已重置')
  }
}

const copyText = (text) => {
  navigator.clipboard.writeText(text)
  ElMessage.success('已复制')
}

onMounted(() => {
  loadDevice()
})
</script>

<style scoped>
.header { margin-bottom: 20px; }
.config-actions { margin-bottom: 10px; }
.data-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  color: #909399;
}
.data-cards { margin-top: 10px; }
.data-card {
  text-align: center;
  margin-bottom: 16px;
}
.data-label {
  font-size: 14px;
  color: #909399;
  margin-bottom: 8px;
}
.data-value {
  font-size: 28px;
  font-weight: 500;
  color: #303133;
}
.data-unit {
  font-size: 14px;
  color: #909399;
  margin-left: 4px;
}
</style>
