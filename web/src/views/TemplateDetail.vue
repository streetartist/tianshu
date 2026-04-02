<template>
  <div class="template-detail" v-loading="loading">
    <div class="header">
      <el-page-header @back="$router.back()">
        <template #content>{{ template.display_name }}</template>
      </el-page-header>
      <el-button type="primary" @click="saveTemplate" v-if="isEditing">保存</el-button>
    </div>

    <el-tabs v-model="activeTab">
      <!-- 基本信息 -->
      <el-tab-pane label="基本信息" name="info">
        <el-form :model="editForm" label-width="100px">
          <el-form-item label="类型标识">
            <el-input v-model="editForm.name" :disabled="true" />
          </el-form-item>
          <el-form-item label="显示名称">
            <el-input v-model="editForm.display_name" :disabled="!isEditing" />
          </el-form-item>
          <el-form-item label="硬件平台">
            <el-select v-model="editForm.hardware_platform" :disabled="!isEditing">
              <el-option label="ESP32" value="esp32" />
              <el-option label="ESP8266" value="esp8266" />
              <el-option label="Arduino" value="arduino" />
              <el-option label="其他" value="other" />
            </el-select>
          </el-form-item>
          <el-form-item label="图标">
            <el-select v-model="editForm.icon" :disabled="!isEditing">
              <el-option label="Monitor" value="Monitor" />
              <el-option label="Odometer" value="Odometer" />
              <el-option label="Cpu" value="Cpu" />
              <el-option label="Connection" value="Connection" />
            </el-select>
          </el-form-item>
          <el-form-item label="描述">
            <el-input v-model="editForm.description" type="textarea" :rows="3" :disabled="!isEditing" />
          </el-form-item>
        </el-form>
      </el-tab-pane>

      <!-- 数据点 -->
      <el-tab-pane label="数据点" name="datapoints">
        <div class="section-header" v-if="isEditing">
          <el-button size="small" type="primary" @click="addDataPoint">添加数据点</el-button>
        </div>
        <el-table :data="dataPoints">
          <el-table-column label="标识" width="140">
            <template #default="{ row }">
              <el-input v-model="row.name" placeholder="如 temperature" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="名称" width="120">
            <template #default="{ row }">
              <el-input v-model="row.label" placeholder="如 温度" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="类型" width="110">
            <template #default="{ row }">
              <el-select v-model="row.type" size="small" :disabled="!isEditing">
                <el-option label="float" value="float" />
                <el-option label="int" value="int" />
                <el-option label="bool" value="bool" />
                <el-option label="string" value="string" />
              </el-select>
            </template>
          </el-table-column>
          <el-table-column label="单位" width="100">
            <template #default="{ row }">
              <el-input v-model="row.unit" placeholder="如 °C" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="操作" width="80" v-if="isEditing">
            <template #default="{ $index }">
              <el-button link type="danger" @click="removeDataPoint($index)">删除</el-button>
            </template>
          </el-table-column>
        </el-table>
      </el-tab-pane>

      <!-- 命令 -->
      <el-tab-pane label="命令" name="commands">
        <div class="section-header" v-if="isEditing">
          <el-button size="small" type="primary" @click="addCommand">添加命令</el-button>
        </div>
        <el-table :data="commands">
          <el-table-column label="标识" width="150">
            <template #default="{ row }">
              <el-input v-model="row.name" placeholder="如 set_led" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="名称" width="150">
            <template #default="{ row }">
              <el-input v-model="row.label" placeholder="如 设置LED" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="参数">
            <template #default="{ row }">
              <el-button link size="small" @click="editCommandParams(row)">
                {{ isEditing ? '编辑参数' : '查看参数' }} ({{ row.params?.length || 0 }})
              </el-button>
            </template>
          </el-table-column>
          <el-table-column label="操作" width="80" v-if="isEditing">
            <template #default="{ $index }">
              <el-button link type="danger" @click="removeCommand($index)">删除</el-button>
            </template>
          </el-table-column>
        </el-table>
      </el-tab-pane>

      <!-- 配置模板 -->
      <el-tab-pane label="配置模板" name="config">
        <h4>通信设置</h4>
        <el-form :model="commSettings" label-width="120px">
          <el-form-item label="心跳间隔(秒)">
            <el-input-number v-model="commSettings.heartbeat_interval" :min="10" :max="600" :disabled="!isEditing" />
          </el-form-item>
          <el-form-item label="数据间隔(秒)">
            <el-input-number v-model="commSettings.data_interval" :min="10" :max="3600" :disabled="!isEditing" />
          </el-form-item>
          <el-form-item label="离线超时(秒)">
            <el-input-number v-model="commSettings.offline_timeout" :min="60" :max="600" :disabled="!isEditing" />
          </el-form-item>
        </el-form>

        <h4 style="margin-top: 20px;">自定义配置项</h4>
        <div class="section-header" v-if="isEditing">
          <el-button size="small" type="primary" @click="addConfigItem">添加配置项</el-button>
        </div>
        <el-table :data="configItems" style="margin-top: 10px;">
          <el-table-column label="配置键" width="150">
            <template #default="{ row }">
              <el-input v-model="row.key" placeholder="如 threshold" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="名称" width="120">
            <template #default="{ row }">
              <el-input v-model="row.label" placeholder="如 阈值" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="类型" width="100">
            <template #default="{ row }">
              <el-select v-model="row.type" size="small" :disabled="!isEditing">
                <el-option label="string" value="string" />
                <el-option label="int" value="int" />
                <el-option label="float" value="float" />
                <el-option label="bool" value="bool" />
              </el-select>
            </template>
          </el-table-column>
          <el-table-column label="默认值" width="120">
            <template #default="{ row }">
              <el-input v-model="row.default_value" placeholder="默认值" size="small" :disabled="!isEditing" />
            </template>
          </el-table-column>
          <el-table-column label="操作" width="80" v-if="isEditing">
            <template #default="{ $index }">
              <el-button link type="danger" @click="removeConfigItem($index)">删除</el-button>
            </template>
          </el-table-column>
        </el-table>
      </el-tab-pane>
    </el-tabs>

    <!-- 命令参数编辑对话框 -->
    <el-dialog v-model="paramsDialog" :title="isEditing ? '编辑命令参数' : '查看命令参数'" width="600">
      <div class="params-header" v-if="isEditing">
        <el-button size="small" type="primary" @click="addParam">添加参数</el-button>
      </div>
      <el-table :data="currentParams" size="small">
        <el-table-column label="参数名" width="120">
          <template #default="{ row }">
            <el-input v-model="row.name" size="small" :disabled="!isEditing" />
          </template>
        </el-table-column>
        <el-table-column label="类型" width="100">
          <template #default="{ row }">
            <el-select v-model="row.type" size="small" :disabled="!isEditing">
              <el-option label="string" value="string" />
              <el-option label="int" value="int" />
              <el-option label="float" value="float" />
              <el-option label="bool" value="bool" />
            </el-select>
          </template>
        </el-table-column>
        <el-table-column label="最小值" width="80">
          <template #default="{ row }">
            <el-input-number v-model="row.min" size="small" controls-position="right"
              v-if="row.type === 'int' || row.type === 'float'" :disabled="!isEditing" />
          </template>
        </el-table-column>
        <el-table-column label="最大值" width="80">
          <template #default="{ row }">
            <el-input-number v-model="row.max" size="small" controls-position="right"
              v-if="row.type === 'int' || row.type === 'float'" :disabled="!isEditing" />
          </template>
        </el-table-column>
        <el-table-column label="操作" width="60" v-if="isEditing">
          <template #default="{ $index }">
            <el-button link type="danger" size="small" @click="removeParam($index)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
      <template #footer>
        <el-button @click="paramsDialog = false">关闭</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import api from '@/api'

const route = useRoute()
const templateId = route.params.id
const isEditing = route.query.edit === '1'

const loading = ref(false)
const activeTab = ref('info')
const template = ref({})

const editForm = reactive({
  name: '',
  display_name: '',
  description: '',
  hardware_platform: 'esp32',
  icon: 'Monitor'
})

const dataPoints = ref([])
const commands = ref([])
const commSettings = reactive({
  heartbeat_interval: 60,
  data_interval: 300,
  offline_timeout: 180
})

// 自定义配置项
const configItems = ref([])

// 命令参数编辑
const paramsDialog = ref(false)
const currentCommand = ref(null)
const currentParams = ref([])

const loadTemplate = async () => {
  loading.value = true
  try {
    const res = await api.templates.get(templateId)
    template.value = res.data || {}
    editForm.name = template.value.name
    editForm.display_name = template.value.display_name
    editForm.description = template.value.description
    editForm.hardware_platform = template.value.hardware_platform
    editForm.icon = template.value.icon
    dataPoints.value = template.value.data_points || []
    commands.value = template.value.commands || []
    configItems.value = template.value.config_items || []
    Object.assign(commSettings, template.value.comm_settings || {})
  } finally {
    loading.value = false
  }
}

const addDataPoint = () => {
  dataPoints.value.push({ name: '', label: '', type: 'float', unit: '' })
}

const removeDataPoint = (index) => {
  dataPoints.value.splice(index, 1)
}

const addCommand = () => {
  commands.value.push({ name: '', label: '', params: [] })
}

const removeCommand = (index) => {
  commands.value.splice(index, 1)
}

const addConfigItem = () => {
  configItems.value.push({ key: '', label: '', type: 'string', default_value: '' })
}

const removeConfigItem = (index) => {
  configItems.value.splice(index, 1)
}

const editCommandParams = (cmd) => {
  currentCommand.value = cmd
  currentParams.value = cmd.params || []
  paramsDialog.value = true
}

const addParam = () => {
  currentParams.value.push({ name: '', type: 'string' })
  currentCommand.value.params = currentParams.value
}

const removeParam = (index) => {
  currentParams.value.splice(index, 1)
  currentCommand.value.params = currentParams.value
}

const saveTemplate = async () => {
  const data = {
    name: editForm.name,
    display_name: editForm.display_name,
    description: editForm.description,
    hardware_platform: editForm.hardware_platform,
    icon: editForm.icon,
    data_points: dataPoints.value,
    commands: commands.value,
    config_items: configItems.value,
    comm_settings: commSettings
  }
  const res = await api.templates.update(templateId, data)
  if (res.code === 0) {
    ElMessage.success('保存成功')
  }
}

onMounted(loadTemplate)
</script>

<style scoped>
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}
.section-header {
  margin-bottom: 10px;
}
.params-header {
  margin-bottom: 10px;
}
</style>
