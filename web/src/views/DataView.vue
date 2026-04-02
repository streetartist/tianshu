<template>
  <div class="data-view">
    <div class="header">
      <h2>数据中心</h2>
      <el-select v-model="selectedDevice" placeholder="选择设备" @change="loadData">
        <el-option
          v-for="d in devices"
          :key="d.id"
          :label="d.name"
          :value="d.id"
        />
      </el-select>
    </div>

    <el-row :gutter="20" v-if="selectedDevice">
      <el-col :span="24">
        <el-card>
          <template #header>最近数据记录</template>
          <el-table :data="records" v-loading="loading" max-height="400">
            <el-table-column prop="timestamp" label="时间" width="180" />
            <el-table-column prop="data_type" label="类型" width="100" />
            <el-table-column label="数据">
              <template #default="{ row }">
                <el-text truncated>{{ JSON.stringify(row.data) }}</el-text>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-col>
    </el-row>

    <el-empty v-else description="请选择设备查看数据" />
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import api from '@/api'

const devices = ref([])
const selectedDevice = ref(null)
const records = ref([])
const loading = ref(false)

const loadDevices = async () => {
  const res = await api.devices.list()
  devices.value = res.data || []
}

const loadData = async () => {
  if (!selectedDevice.value) return
  loading.value = true
  try {
    const res = await api.data.get(selectedDevice.value, { limit: 50 })
    records.value = res.data || []
  } finally {
    loading.value = false
  }
}

onMounted(loadDevices)
</script>

<style scoped>
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}
.header h2 { margin: 0; }
</style>
