<template>
  <div class="devices">
    <div class="header">
      <h2>设备管理</h2>
      <div class="header-actions">
        <el-button @click="refreshStatus" :loading="refreshing">
          <el-icon><Refresh /></el-icon> 刷新状态
        </el-button>
        <el-button type="primary" @click="showDialog = true">
          <el-icon><Plus /></el-icon> 添加设备
        </el-button>
      </div>
    </div>

    <el-table :data="devices" v-loading="loading">
      <el-table-column prop="name" label="设备名称" />
      <el-table-column prop="device_type" label="类型" width="120" />
      <el-table-column prop="device_id" label="设备ID" width="300" />
      <el-table-column prop="status" label="状态" width="100">
        <template #default="{ row }">
          <el-tag :type="row.status === 'online' ? 'success' : 'info'">
            {{ row.status === 'online' ? '在线' : '离线' }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column prop="last_seen" label="最后在线" width="180" />
      <el-table-column label="操作" width="150">
        <template #default="{ row }">
          <el-button link type="primary" @click="viewDevice(row)">详情</el-button>
          <el-button link type="danger" @click="deleteDevice(row)">删除</el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 添加设备对话框 -->
    <el-dialog v-model="showDialog" title="添加设备" width="500">
      <el-form :model="form" label-width="80px">
        <el-form-item label="名称">
          <el-input v-model="form.name" placeholder="设备名称" />
        </el-form-item>
        <el-form-item label="类型">
          <el-select v-model="form.device_type" style="width: 100%">
            <el-option
              v-for="t in templates"
              :key="t.name"
              :label="t.display_name"
              :value="t.name"
            />
            <el-option label="其他" value="other" />
          </el-select>
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="form.description" type="textarea" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showDialog = false">取消</el-button>
        <el-button type="primary" @click="createDevice">确定</el-button>
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
const devices = ref([])
const templates = ref([])
const loading = ref(false)
const refreshing = ref(false)
const showDialog = ref(false)

const form = reactive({
  name: '',
  device_type: '',
  description: ''
})

const loadDevices = async () => {
  loading.value = true
  try {
    const res = await api.devices.list()
    devices.value = res.data || []
  } finally {
    loading.value = false
  }
}

const refreshStatus = async () => {
  refreshing.value = true
  try {
    await api.devices.checkStatus()
    await loadDevices()
    ElMessage.success('状态已刷新')
  } finally {
    refreshing.value = false
  }
}

const createDevice = async () => {
  const res = await api.devices.create(form)
  if (res.code === 0) {
    ElMessage.success('设备创建成功')
    showDialog.value = false
    loadDevices()
  }
}

const viewDevice = (row) => {
  router.push(`/devices/${row.device_id}`)
}

const deleteDevice = async (row) => {
  try {
    await ElMessageBox.confirm('确定要删除该设备吗？', '提示', {
      type: 'warning'
    })
    const res = await api.devices.delete(row.device_id)
    if (res.code === 0) {
      ElMessage.success('设备已删除')
      loadDevices()
    }
  } catch (e) {
    // User cancelled
  }
}

const loadTemplates = async () => {
  const res = await api.templates.list()
  templates.value = res.data || []
}

onMounted(() => {
  loadDevices()
  loadTemplates()
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
.header-actions {
  display: flex;
  gap: 10px;
}
</style>
