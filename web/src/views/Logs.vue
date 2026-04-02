<template>
  <div class="logs">
    <h2>操作日志</h2>

    <el-table :data="logs" v-loading="loading">
      <el-table-column prop="created_at" label="时间" width="180" />
      <el-table-column prop="username" label="用户" width="120" />
      <el-table-column prop="action" label="操作" width="120" />
      <el-table-column prop="resource_type" label="资源类型" width="120" />
      <el-table-column prop="ip_address" label="IP地址" width="140" />
      <el-table-column label="详情">
        <template #default="{ row }">
          {{ row.detail ? JSON.stringify(row.detail) : '-' }}
        </template>
      </el-table-column>
    </el-table>

    <el-pagination
      v-model:current-page="page"
      :page-size="20"
      :total="total"
      @current-change="loadLogs"
      style="margin-top: 20px"
    />
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import api from '@/api'

const logs = ref([])
const loading = ref(false)
const page = ref(1)
const total = ref(0)

const loadLogs = async () => {
  loading.value = true
  try {
    const res = await api.logs.list({ page: page.value })
    logs.value = res.data?.items || []
    total.value = res.data?.total || 0
  } finally {
    loading.value = false
  }
}

onMounted(loadLogs)
</script>

<style scoped>
.logs h2 { margin: 0 0 20px; }
</style>
