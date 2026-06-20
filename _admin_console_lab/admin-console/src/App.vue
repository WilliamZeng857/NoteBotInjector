<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'

type TabKey = 'overview' | 'licenses' | 'devices' | 'artifacts' | 'events'

type Artifact = {
  category: string
  artifactType: string
  fileName: string
  version: string
  versionCode: number
  enabled: boolean
  required: boolean
  protocolMin: number
  protocolMax: number
  size: number
  md5: string
  sha256: string
  publishedAtUtc: string
  uploadSource: string
}

type License = {
  keyId: string
  keyPlaintextNormalized: string
  tier: string
  tierValue: number
  featureFlags: number
  revoked: boolean
  maxDevices: number
  boundQQ: string
  nickname: string
  avatarUrl: string
  boundDllName: string
  boundDllSha256: string
  durationDays: number
  activatedAtUtc: string | null
  expiresAtUtc: string | null
  updatedAtUtc: string
  note: string
  deviceCount?: number
}

type Device = {
  keyId: string
  keyPlaintextNormalized: string
  deviceId: string
  machineLabel: string
  status: string
  firstActivatedAtUtc: string
  lastSeenAtUtc: string
}

type EventItem = {
  id: string
  tsUtc: string
  category: string
  action: string
  status: string
  message: string
}

type RuntimeReport = {
  id: string
  receivedAtUtc: string
  keyId: string
  deviceId: string
  clientKind: string
  channel: string
  currentMainVersion: string
  currentAuthDllVersion: string
  currentUpdaterVersion: string
  protocolVersion: number
  runtimeInfo?: Record<string, string | number>
}

type Overview = {
  service: string
  protocolVersion: number
  abiVersion: number
  counts: {
    licenses: number
    onlineDevices: number
    alerts: number
    artifacts: number
  }
  latestArtifacts: {
    authDll: Artifact | null
    overlayDll: Artifact | null
    updaterExe: Artifact | null
  }
  settings: {
    downloadMode: string
  }
}

const activeTab = ref<TabKey>('overview')
const loading = ref(false)
const banner = reactive({ tone: 'info', text: '正在准备本地原型数据...' })

const overview = ref<Overview | null>(null)
const licenses = ref<License[]>([])
const devices = ref<Device[]>([])
const artifacts = ref<Artifact[]>([])
const events = ref<EventItem[]>([])
const reports = ref<RuntimeReport[]>([])
const settingsForm = reactive({
  downloadMode: 'server-url',
})

const search = ref('')
const selectedKeyId = ref('')
const selectedLicense = ref<License | null>(null)

const createKeyForm = reactive({
  keyPlaintextNormalized: '',
  tier: 'Premium',
  boundQQ: '',
  featureFlags: 16,
  maxDevices: 1,
  durationDays: 30,
  note: '',
})

const editForm = reactive({
  keyId: '',
  tier: 'Premium',
  boundQQ: '',
  nickname: '',
  featureFlags: 0,
  maxDevices: 1,
  revoked: false,
  boundDllName: '',
  expiresAtUtc: '',
  note: '',
})

const batchBindForm = reactive({
  tier: '',
  boundDllName: '',
})

const artifactUploadForms = reactive({
  auth: {
    category: 'bootstrap',
    artifactType: 'auth_dll',
    version: '',
    versionCode: 0,
    protocolMin: 3,
    protocolMax: 3,
    file: null as File | null,
  },
  overlay: {
    category: 'notebot',
    artifactType: 'overlay_dll',
    version: '',
    versionCode: 0,
    protocolMin: 3,
    protocolMax: 3,
    file: null as File | null,
  },
  advanced: {
    category: 'bootstrap',
    artifactType: 'main_exe',
    version: '',
    versionCode: 0,
    protocolMin: 3,
    protocolMax: 3,
    file: null as File | null,
  },
})

const uploadState = reactive({
  auth: { active: false, label: '等待上传', progress: 0 },
  overlay: { active: false, label: '等待上传', progress: 0 },
  advanced: { active: false, label: '等待上传', progress: 0 },
})

const tabs: Array<{ key: TabKey; title: string; hint: string }> = [
  { key: 'overview', title: '总览', hint: '先给人结论，不先糊一脸内部字段' },
  { key: 'licenses', title: '密钥管理', hint: 'QQ、版本绑定、权限和到期一屏看懂' },
  { key: 'devices', title: '设备追踪', hint: '谁在线、谁冷却、谁值得处理' },
  { key: 'artifacts', title: '产物发布', hint: '上传进度、版本识别、启停一体化' },
  { key: 'events', title: '动态日志', hint: '让你知道客户和系统到底在干什么' },
]

const filteredLicenses = computed(() => {
  const keyword = search.value.trim().toLowerCase()
  if (!keyword) {
    return licenses.value
  }
  return licenses.value.filter((item) =>
    [item.keyPlaintextNormalized, item.boundQQ, item.nickname, item.boundDllName].join(' ').toLowerCase().includes(keyword),
  )
})

const selectedDevices = computed(() => devices.value.filter((item) => item.keyId === selectedKeyId.value))
const selectedLicenseArtifacts = computed(() =>
  artifacts.value.filter((item) => item.artifactType === 'overlay_dll' || item.artifactType === 'auth_dll'),
)

const topCustomers = computed(() =>
  [...licenses.value]
    .sort((a, b) => (b.deviceCount ?? 0) - (a.deviceCount ?? 0))
    .slice(0, 3),
)

const api = async <T>(path: string, options?: RequestInit) => {
  const response = await fetch(path, {
    headers: {
      'Content-Type': 'application/json',
      ...(options?.headers ?? {}),
    },
    ...options,
  })
  const payload = await response.json()
  if (!response.ok || payload.status !== 'ok') {
    throw new Error(payload.message || `请求失败: ${response.status}`)
  }
  return payload.data as T
}

const setBanner = (text: string, tone: 'info' | 'success' | 'warning' | 'error' = 'info') => {
  banner.text = text
  banner.tone = tone
}

const formatTime = (value?: string | null) => {
  if (!value) return '未提供'
  const date = new Date(value)
  if (Number.isNaN(date.getTime())) return value
  return date.toLocaleString('zh-CN', { hour12: false })
}

const formatSize = (value: number) => {
  if (!value) return '0 B'
  if (value < 1024) return `${value} B`
  if (value < 1024 * 1024) return `${(value / 1024).toFixed(1)} KB`
  return `${(value / 1024 / 1024).toFixed(2)} MB`
}

const statusTone = (status: string) => {
  if (status === 'online' || status === 'ok') return 'success'
  if (status === 'cooldown' || status === 'idle') return 'warning'
  if (status === 'revoked' || status === 'error') return 'danger'
  return 'neutral'
}

const detectVersion = (fileName: string) => {
  const match = fileName.match(/v?(\d+\.\d+\.\d+(?:[._-]\d+)?)/i)
  if (!match) {
    return { version: '', versionCode: 0 }
  }
  const normalized = match[1].replace(/_/g, '.')
  const parts = normalized.split(/[.-]/).map((item) => Number(item) || 0)
  const [major = 0, minor = 0, patch = 0, build = 0] = parts
  return {
    version: normalized.startsWith('v') ? normalized : normalized,
    versionCode: major * 1_000_000 + minor * 10_000 + patch * 100 + build,
  }
}

const refreshAll = async () => {
  loading.value = true
  try {
    const [overviewData, licenseData, deviceData, artifactData, eventData, reportData] = await Promise.all([
      api<Overview>('/api/overview'),
      api<License[]>('/api/licenses'),
      api<Device[]>('/api/devices'),
      api<Artifact[]>('/api/artifacts'),
      api<EventItem[]>('/api/events'),
      api<RuntimeReport[]>('/api/runtime-reports'),
    ])
    overview.value = overviewData
    settingsForm.downloadMode = overviewData.settings?.downloadMode || 'server-url'
    licenses.value = licenseData
    devices.value = deviceData
    artifacts.value = artifactData
    events.value = eventData
    reports.value = reportData
    if (!selectedKeyId.value && licenses.value[0]) {
      selectedKeyId.value = licenses.value[0].keyId
    }
    if (selectedLicense.value) {
      selectedLicense.value = licenses.value.find((item) => item.keyId === selectedLicense.value?.keyId) ?? null
    }
    setBanner('本地原型数据已刷新，你现在看到的是一套真正按用户理解组织的后台。', 'success')
  } catch (error) {
    setBanner(error instanceof Error ? error.message : '读取数据失败', 'error')
  } finally {
    loading.value = false
  }
}

const randomKey = () => {
  const part = () => Math.floor(Math.random() * 0xffffffff).toString(16).toUpperCase().padStart(8, '0')
  createKeyForm.keyPlaintextNormalized = `NB-${part()}-${part()}`
}

const createLicense = async () => {
  await api('/api/licenses', {
    method: 'POST',
    body: JSON.stringify(createKeyForm),
  })
  setBanner('新密钥已经加到列表里了。', 'success')
  createKeyForm.keyPlaintextNormalized = ''
  createKeyForm.boundQQ = ''
  createKeyForm.note = ''
  await refreshAll()
}

const openEdit = (license: License) => {
  selectedLicense.value = license
  Object.assign(editForm, {
    keyId: license.keyId,
    tier: license.tier,
    boundQQ: license.boundQQ,
    nickname: license.nickname,
    featureFlags: license.featureFlags,
    maxDevices: license.maxDevices,
    revoked: license.revoked,
    boundDllName: license.boundDllName,
    expiresAtUtc: license.expiresAtUtc ?? '',
    note: license.note,
  })
}

const saveEdit = async () => {
  await api(`/api/licenses/${editForm.keyId}`, {
    method: 'PATCH',
    body: JSON.stringify(editForm),
  })
  setBanner('客户资料已经更新。', 'success')
  await refreshAll()
}

const removeLicense = async (license: License) => {
  if (!window.confirm(`确定删除 ${license.keyPlaintextNormalized} 吗？`)) return
  await api(`/api/licenses/${license.keyId}`, { method: 'DELETE' })
  selectedLicense.value = null
  setBanner('密钥和它的设备记录已经移除。', 'warning')
  await refreshAll()
}

const batchBindDll = async () => {
  await api('/api/licenses/batch-bind-overlay', {
    method: 'POST',
    body: JSON.stringify(batchBindForm),
  })
  setBanner('批量 DLL 绑定已经完成。', 'success')
  await refreshAll()
}

const removeDevice = async (device: Device) => {
  await api(`/api/devices/${device.keyId}/${device.deviceId}`, { method: 'DELETE' })
  setBanner(`设备 ${device.machineLabel} 已移除。`, 'warning')
  await refreshAll()
}

const resetDevices = async () => {
  if (!selectedKeyId.value) return
  await api(`/api/devices/${selectedKeyId.value}/reset`, { method: 'POST' })
  setBanner('这把密钥的设备已全部重置。', 'warning')
  await refreshAll()
}

const toggleArtifact = async (artifact: Artifact) => {
  await api(`/api/artifacts/${artifact.sha256}/state`, {
    method: 'PATCH',
    body: JSON.stringify({ enabled: !artifact.enabled }),
  })
  setBanner(`${artifact.fileName} 已${artifact.enabled ? '停用' : '启用'}。`, 'success')
  await refreshAll()
}

const saveDownloadMode = async () => {
  await api('/api/settings/download-mode', {
    method: 'PATCH',
    body: JSON.stringify({ downloadMode: settingsForm.downloadMode }),
  })
  setBanner('下载模式已经改好了。', 'success')
  await refreshAll()
}

const onPickFile = (kind: keyof typeof artifactUploadForms, event: Event) => {
  const file = (event.target as HTMLInputElement).files?.[0] ?? null
  artifactUploadForms[kind].file = file
  if (!file) return
  const detected = detectVersion(file.name)
  if (!artifactUploadForms[kind].version) artifactUploadForms[kind].version = detected.version
  if (!artifactUploadForms[kind].versionCode) artifactUploadForms[kind].versionCode = detected.versionCode
}

const uploadArtifact = async (kind: keyof typeof artifactUploadForms) => {
  const current = artifactUploadForms[kind]
  if (!current.file) {
    setBanner('先选文件，再谈上传。', 'warning')
    return
  }

  const formData = new FormData()
  formData.append('category', current.category)
  formData.append('artifactType', current.artifactType)
  formData.append('version', current.version)
  formData.append('versionCode', String(current.versionCode))
  formData.append('protocolMin', String(current.protocolMin))
  formData.append('protocolMax', String(current.protocolMax))
  formData.append('file', current.file)

  uploadState[kind].active = true
  uploadState[kind].label = '正在上传...'
  uploadState[kind].progress = 0

  await new Promise<void>((resolve, reject) => {
    const xhr = new XMLHttpRequest()
    xhr.open('POST', '/api/artifacts/upload')
    xhr.upload.onprogress = (event) => {
      if (!event.lengthComputable) return
      uploadState[kind].progress = Math.round((event.loaded / event.total) * 100)
      uploadState[kind].label = uploadState[kind].progress >= 100 ? '服务器处理中...' : '正在上传...'
    }
    xhr.onload = () => {
      try {
        const payload = JSON.parse(xhr.responseText)
        if (xhr.status >= 400 || payload.status !== 'ok') {
          throw new Error(payload.message || '上传失败')
        }
        uploadState[kind].progress = 100
        uploadState[kind].label = '上传完成'
        resolve()
      } catch (error) {
        reject(error)
      }
    }
    xhr.onerror = () => reject(new Error('网络异常，上传没有到服务端'))
    xhr.send(formData)
  })
    .then(async () => {
      setBanner(`${current.file?.name} 已进入产物列表。`, 'success')
      current.file = null
      await refreshAll()
    })
    .catch((error) => {
      setBanner(error instanceof Error ? error.message : '上传失败', 'error')
      uploadState[kind].label = '上传失败'
    })
    .finally(() => {
      setTimeout(() => {
        uploadState[kind].active = false
      }, 800)
    })
}

onMounted(async () => {
  randomKey()
  await refreshAll()
})
</script>

<template>
  <div class="app-shell">
    <div class="background-grid"></div>

    <aside class="sidebar">
      <div class="brand-card">
        <p class="eyebrow">NOTEbot Admin Lab</p>
        <h1>把后台先做成给人用的</h1>
        <p class="brand-copy">
          这套本地原型不碰远端现网，只验证一件事：新的管理后台能不能真正从用户视角出发，
          把密钥、QQ、设备、版本、上传和风险状态都讲清楚。
        </p>
      </div>

      <nav class="nav-list">
        <button
          v-for="tab in tabs"
          :key="tab.key"
          :class="['nav-item', { active: activeTab === tab.key }]"
          type="button"
          @click="activeTab = tab.key"
        >
          <span>{{ tab.title }}</span>
          <small>{{ tab.hint }}</small>
        </button>
      </nav>

      <section class="side-panel">
        <h3>这版重点</h3>
        <ul>
          <li>上传时有真实进度条，不再靠猜。</li>
          <li>QQ 号直接带头像，先看人，再看密钥。</li>
          <li>版本字符串和版本号尽量从文件名自动识别。</li>
          <li>先在本地 mock 跑顺，再决定如何替换远端旧前端。</li>
        </ul>
      </section>
    </aside>

    <main class="main-shell">
      <header class="topbar">
        <div>
          <p class="eyebrow">本地原型 · Vue + Mock API</p>
          <h2>NoteBot 管理控制台重设计</h2>
        </div>
        <div class="topbar-actions">
          <input v-model="search" class="search-input" placeholder="搜密钥、QQ、昵称、DLL" />
          <button class="btn ghost" :disabled="loading" @click="refreshAll">刷新数据</button>
        </div>
      </header>

      <div :class="['banner', banner.tone]">
        {{ banner.text }}
      </div>

      <section v-if="activeTab === 'overview'" class="page">
        <div class="stats-grid">
          <article class="stat-card">
            <label>密钥总量</label>
            <strong>{{ overview?.counts.licenses ?? 0 }}</strong>
            <span>包含试用、正式和开发账户</span>
          </article>
          <article class="stat-card">
            <label>在线设备</label>
            <strong>{{ overview?.counts.onlineDevices ?? 0 }}</strong>
            <span>优先帮助真正在线的人</span>
          </article>
          <article class="stat-card">
            <label>风险提示</label>
            <strong>{{ overview?.counts.alerts ?? 0 }}</strong>
            <span>未激活、撤销、冷却中的对象</span>
          </article>
          <article class="stat-card">
            <label>产物数量</label>
            <strong>{{ overview?.counts.artifacts ?? 0 }}</strong>
            <span>验证层、NoteBot DLL、updater 一起看</span>
          </article>
        </div>

        <div class="page-grid">
          <section class="panel span-7">
            <div class="panel-head">
              <div>
                <h3>客户视角状态板</h3>
                <p>谁最活跃、谁最需要看、谁的 DLL 绑定最值得先处理</p>
              </div>
            </div>

            <div class="customer-stack">
              <article v-for="user in topCustomers" :key="user.keyId" class="customer-card" @click="openEdit(user)">
                <img :src="user.avatarUrl" class="avatar" :alt="user.nickname" />
                <div class="customer-main">
                  <div class="customer-row">
                    <strong>{{ user.nickname }}</strong>
                    <span :class="['pill', statusTone(user.revoked ? 'error' : user.activatedAtUtc ? 'ok' : 'idle')]">
                      {{ user.revoked ? '已撤销' : user.activatedAtUtc ? '已激活' : '未激活' }}
                    </span>
                  </div>
                  <p>{{ user.boundQQ }} · {{ user.keyPlaintextNormalized }}</p>
                  <small>绑定 DLL：{{ user.boundDllName || '暂未指定' }} · 设备数 {{ user.deviceCount ?? 0 }}</small>
                </div>
              </article>
            </div>
          </section>

          <section class="panel span-5">
            <div class="panel-head">
              <div>
                <h3>当前发布位</h3>
                <p>不用手动翻日志，当前对外提供什么一眼就懂</p>
              </div>
            </div>

            <div class="release-stack">
              <article v-for="artifact in artifacts.slice(0, 3)" :key="artifact.sha256" class="release-card">
                <div class="release-row">
                  <strong>{{ artifact.fileName }}</strong>
                  <span :class="['pill', artifact.enabled ? 'success' : 'danger']">
                    {{ artifact.enabled ? '启用中' : '已停用' }}
                  </span>
                </div>
                <p>{{ artifact.artifactType }} · {{ artifact.version || '未识别版本' }}</p>
                <small>{{ formatSize(artifact.size) }} · 协议 {{ artifact.protocolMin }}-{{ artifact.protocolMax }}</small>
              </article>
            </div>
          </section>

          <section class="panel span-12">
            <div class="panel-head">
              <div>
                <h3>最近动态</h3>
                <p>让你知道后台和客户最近到底在干什么</p>
              </div>
            </div>

            <div class="timeline">
              <article v-for="event in events.slice(0, 6)" :key="event.id" class="timeline-item">
                <div class="timeline-dot"></div>
                <div class="timeline-body">
                  <div class="timeline-row">
                    <strong>{{ event.category }} / {{ event.action }}</strong>
                    <span>{{ formatTime(event.tsUtc) }}</span>
                  </div>
                  <p>{{ event.message }}</p>
                </div>
              </article>
            </div>
          </section>
        </div>
      </section>

      <section v-else-if="activeTab === 'licenses'" class="page">
        <div class="page-grid">
          <section class="panel span-4">
            <div class="panel-head">
              <div>
                <h3>新建密钥</h3>
                <p>基础字段尽量少手填，先把常用路径做顺手</p>
              </div>
            </div>
            <div class="form-grid">
              <label>密钥</label>
              <div class="inline-field">
                <input v-model="createKeyForm.keyPlaintextNormalized" />
                <button class="btn ghost small" @click="randomKey">随机生成</button>
              </div>
              <label>绑定 QQ</label>
              <input v-model="createKeyForm.boundQQ" placeholder="输入 QQ 号自动带头像" />
              <label>等级</label>
              <select v-model="createKeyForm.tier">
                <option value="Trial">Trial</option>
                <option value="Premium">Premium</option>
                <option value="Dev">Dev</option>
              </select>
              <label>功能位</label>
              <input v-model.number="createKeyForm.featureFlags" type="number" min="0" />
              <label>最大设备数</label>
              <input v-model.number="createKeyForm.maxDevices" type="number" min="1" />
              <label>有效天数</label>
              <input v-model.number="createKeyForm.durationDays" type="number" min="0" />
              <label>备注</label>
              <textarea v-model="createKeyForm.note" rows="3" placeholder="例如：白名单、联调、观察中"></textarea>
              <button class="btn primary" @click="createLicense">创建密钥</button>
            </div>
          </section>

          <section class="panel span-8">
            <div class="panel-head">
              <div>
                <h3>密钥列表</h3>
                <p>先看 QQ 头像和状态，再看密钥和 DLL，不把人藏在字段后面</p>
              </div>
            </div>

            <div class="license-list">
              <article
                v-for="license in filteredLicenses"
                :key="license.keyId"
                class="license-card"
                @click="openEdit(license)"
              >
                <img :src="license.avatarUrl" class="avatar large" :alt="license.nickname" />
                <div class="license-main">
                  <div class="customer-row">
                    <strong>{{ license.nickname }}</strong>
                    <div class="row-gap">
                      <span :class="['pill', 'neutral']">{{ license.tier }}</span>
                      <span :class="['pill', statusTone(license.revoked ? 'error' : license.activatedAtUtc ? 'ok' : 'idle')]">
                        {{ license.revoked ? '已撤销' : license.activatedAtUtc ? '已激活' : '未激活' }}
                      </span>
                    </div>
                  </div>
                  <p>{{ license.boundQQ }} · {{ license.keyPlaintextNormalized }}</p>
                  <small>绑定 DLL：{{ license.boundDllName || '未指定' }} · 设备 {{ license.deviceCount ?? 0 }} 台</small>
                </div>
                <button class="btn ghost small danger" @click.stop="removeLicense(license)">删除</button>
              </article>
            </div>
          </section>

          <section class="panel span-12" v-if="selectedLicense">
            <div class="panel-head">
              <div>
                <h3>客户资料编辑</h3>
                <p>{{ selectedLicense.keyPlaintextNormalized }} 的高频字段在这里直接改</p>
              </div>
            </div>
            <div class="edit-layout">
              <div class="edit-preview">
                <img :src="selectedLicense.avatarUrl" class="avatar xl" :alt="selectedLicense.nickname" />
                <strong>{{ selectedLicense.nickname }}</strong>
                <p>{{ selectedLicense.boundQQ }}</p>
                <small>最近更新时间：{{ formatTime(selectedLicense.updatedAtUtc) }}</small>
              </div>
              <div class="form-grid edit-form">
                <label>昵称</label>
                <input v-model="editForm.nickname" />
                <label>绑定 QQ</label>
                <input v-model="editForm.boundQQ" />
                <label>等级</label>
                <select v-model="editForm.tier">
                  <option value="Trial">Trial</option>
                  <option value="Premium">Premium</option>
                  <option value="Dev">Dev</option>
                </select>
                <label>功能位</label>
                <input v-model.number="editForm.featureFlags" type="number" min="0" />
                <label>最大设备数</label>
                <input v-model.number="editForm.maxDevices" type="number" min="1" />
                <label>绑定 DLL</label>
                <select v-model="editForm.boundDllName">
                  <option value="">暂不指定</option>
                  <option v-for="artifact in selectedLicenseArtifacts" :key="artifact.sha256" :value="artifact.fileName">
                    {{ artifact.fileName }}
                  </option>
                </select>
                <label>到期时间</label>
                <input v-model="editForm.expiresAtUtc" placeholder="2026-07-02T09:07:02.000Z" />
                <label>备注</label>
                <textarea v-model="editForm.note" rows="3"></textarea>
                <label>状态</label>
                <div class="inline-toggle">
                  <input id="revoked" v-model="editForm.revoked" type="checkbox" />
                  <label for="revoked">标记为已撤销</label>
                </div>
                <button class="btn primary" @click="saveEdit">保存客户资料</button>
              </div>
            </div>
          </section>

          <section class="panel span-12">
            <div class="panel-head">
              <div>
                <h3>批量绑定 NoteBot DLL</h3>
                <p>当你换一版 DLL 时，不该一条一条点</p>
              </div>
            </div>
            <div class="batch-row">
              <select v-model="batchBindForm.tier">
                <option value="">全部等级</option>
                <option value="Trial">Trial</option>
                <option value="Premium">Premium</option>
                <option value="Dev">Dev</option>
              </select>
              <select v-model="batchBindForm.boundDllName">
                <option value="">选择要批量绑定的 DLL</option>
                <option v-for="artifact in artifacts.filter((item) => item.artifactType === 'overlay_dll')" :key="artifact.sha256" :value="artifact.fileName">
                  {{ artifact.fileName }}
                </option>
              </select>
              <button class="btn ghost" @click="batchBindDll">批量绑定</button>
            </div>
          </section>
        </div>
      </section>

      <section v-else-if="activeTab === 'devices'" class="page">
        <div class="page-grid">
          <section class="panel span-4">
            <div class="panel-head">
              <div>
                <h3>设备关注对象</h3>
                <p>先选客户，再看这把密钥下的机器</p>
              </div>
            </div>
            <select v-model="selectedKeyId" class="wide-select">
              <option v-for="license in licenses" :key="license.keyId" :value="license.keyId">
                {{ license.nickname }} · {{ license.keyPlaintextNormalized }}
              </option>
            </select>
            <button class="btn danger wide-button" @click="resetDevices">重置这把密钥的全部设备</button>
          </section>

          <section class="panel span-8">
            <div class="panel-head">
              <div>
                <h3>设备列表</h3>
                <p>带机器标签、在线状态和最后出现时间</p>
              </div>
            </div>
            <div class="device-table">
              <article v-for="device in selectedDevices" :key="device.deviceId" class="device-row">
                <div>
                  <strong>{{ device.machineLabel }}</strong>
                  <p>{{ device.deviceId }}</p>
                  <small>首次激活：{{ formatTime(device.firstActivatedAtUtc) }}</small>
                </div>
                <div class="device-side">
                  <span :class="['pill', statusTone(device.status)]">{{ device.status }}</span>
                  <small>最近出现：{{ formatTime(device.lastSeenAtUtc) }}</small>
                </div>
                <button class="btn ghost small danger" @click="removeDevice(device)">移除设备</button>
              </article>
              <div v-if="!selectedDevices.length" class="empty-state">这把密钥当前没有设备记录。</div>
            </div>
          </section>
        </div>
      </section>

      <section v-else-if="activeTab === 'artifacts'" class="page">
        <div class="page-grid">
          <section class="panel span-4">
            <div class="panel-head">
              <div>
                <h3>上传验证层 DLL</h3>
                <p>文件名能识别版本的，就尽量替你识别，不逼你手打。</p>
              </div>
            </div>
            <div class="form-grid">
              <label>选择文件</label>
              <input type="file" accept=".dll" @change="onPickFile('auth', $event)" />
              <label>版本字符串</label>
              <input v-model="artifactUploadForms.auth.version" />
              <label>版本号</label>
              <input v-model.number="artifactUploadForms.auth.versionCode" type="number" />
              <button class="btn primary" @click="uploadArtifact('auth')">上传验证层 DLL</button>
              <div v-if="uploadState.auth.active" class="upload-progress">
                <div class="progress-row">
                  <span>{{ uploadState.auth.label }}</span>
                  <strong>{{ uploadState.auth.progress }}%</strong>
                </div>
                <div class="progress-track">
                  <div class="progress-fill" :style="{ width: `${uploadState.auth.progress}%` }"></div>
                </div>
              </div>
            </div>
          </section>

          <section class="panel span-4">
            <div class="panel-head">
              <div>
                <h3>上传 NoteBot DLL</h3>
                <p>按密钥绑定的业务 DLL，和验证层分开看，用户也更好理解。</p>
              </div>
            </div>
            <div class="form-grid">
              <label>选择文件</label>
              <input type="file" accept=".dll" @change="onPickFile('overlay', $event)" />
              <label>版本字符串</label>
              <input v-model="artifactUploadForms.overlay.version" />
              <label>版本号</label>
              <input v-model.number="artifactUploadForms.overlay.versionCode" type="number" />
              <button class="btn primary" @click="uploadArtifact('overlay')">上传 NoteBot DLL</button>
              <div v-if="uploadState.overlay.active" class="upload-progress">
                <div class="progress-row">
                  <span>{{ uploadState.overlay.label }}</span>
                  <strong>{{ uploadState.overlay.progress }}%</strong>
                </div>
                <div class="progress-track">
                  <div class="progress-fill" :style="{ width: `${uploadState.overlay.progress}%` }"></div>
                </div>
              </div>
            </div>
          </section>

          <section class="panel span-4">
            <div class="panel-head">
              <div>
                <h3>高级上传</h3>
                <p>给主 EXE 或 updater 留扩展位，但默认不让它抢主流程。</p>
              </div>
            </div>
            <div class="form-grid">
              <label>类别</label>
              <select v-model="artifactUploadForms.advanced.category">
                <option value="bootstrap">bootstrap</option>
                <option value="notebot">notebot</option>
              </select>
              <label>产物类型</label>
              <select v-model="artifactUploadForms.advanced.artifactType">
                <option value="main_exe">main_exe</option>
                <option value="updater_exe">updater_exe</option>
                <option value="auth_dll">auth_dll</option>
                <option value="overlay_dll">overlay_dll</option>
              </select>
              <label>选择文件</label>
              <input type="file" @change="onPickFile('advanced', $event)" />
              <label>版本字符串</label>
              <input v-model="artifactUploadForms.advanced.version" />
              <label>版本号</label>
              <input v-model.number="artifactUploadForms.advanced.versionCode" type="number" />
              <button class="btn ghost" @click="uploadArtifact('advanced')">执行高级上传</button>
              <div v-if="uploadState.advanced.active" class="upload-progress">
                <div class="progress-row">
                  <span>{{ uploadState.advanced.label }}</span>
                  <strong>{{ uploadState.advanced.progress }}%</strong>
                </div>
                <div class="progress-track">
                  <div class="progress-fill" :style="{ width: `${uploadState.advanced.progress}%` }"></div>
                </div>
              </div>
            </div>
          </section>

          <section class="panel span-12">
            <div class="panel-head">
              <div>
                <h3>发布清单</h3>
                <p>大小、版本、协议范围、启停状态一起看，不用来回翻。</p>
              </div>
              <div class="download-mode">
                <select v-model="settingsForm.downloadMode">
                  <option value="server-url">server-url</option>
                  <option value="signed-url">signed-url</option>
                </select>
                <button class="btn ghost small" @click="saveDownloadMode">保存下载模式</button>
              </div>
            </div>
            <div class="artifact-grid">
              <article v-for="artifact in artifacts" :key="artifact.sha256" class="artifact-card">
                <div class="customer-row">
                  <strong>{{ artifact.fileName }}</strong>
                  <span :class="['pill', artifact.enabled ? 'success' : 'danger']">
                    {{ artifact.enabled ? '启用中' : '已停用' }}
                  </span>
                </div>
                <p>{{ artifact.artifactType }} · {{ artifact.version || '未识别版本' }}</p>
                <small>{{ formatSize(artifact.size) }} · SHA {{ artifact.sha256.slice(0, 12) }}...</small>
                <div class="artifact-meta">
                  <span>协议 {{ artifact.protocolMin }}-{{ artifact.protocolMax }}</span>
                  <span>{{ formatTime(artifact.publishedAtUtc) }}</span>
                </div>
                <button class="btn ghost small" @click="toggleArtifact(artifact)">
                  {{ artifact.enabled ? '停用' : '启用' }}
                </button>
              </article>
            </div>
          </section>
        </div>
      </section>

      <section v-else class="page">
        <div class="page-grid">
          <section class="panel span-7">
            <div class="panel-head">
              <div>
                <h3>事件流</h3>
                <p>先看含义，再看动作名，不强迫用户理解后端内部术语。</p>
              </div>
            </div>
            <div class="timeline">
              <article v-for="event in events" :key="event.id" class="timeline-item">
                <div class="timeline-dot"></div>
                <div class="timeline-body">
                  <div class="timeline-row">
                    <strong>{{ event.category }} / {{ event.action }}</strong>
                    <span :class="['pill', statusTone(event.status)]">{{ event.status }}</span>
                  </div>
                  <p>{{ event.message }}</p>
                  <small>{{ formatTime(event.tsUtc) }}</small>
                </div>
              </article>
            </div>
          </section>

          <section class="panel span-5">
            <div class="panel-head">
              <div>
                <h3>客户端运行上报</h3>
                <p>不是无意义硬件堆砌，而是帮你判断用户现在到底跑的什么。</p>
              </div>
            </div>
            <div class="report-stack">
              <article v-for="report in reports" :key="report.id" class="report-card">
                <div class="customer-row">
                  <strong>{{ report.clientKind }}</strong>
                  <span class="pill neutral">{{ report.channel }}</span>
                </div>
                <p>{{ report.keyId }} · {{ report.deviceId }}</p>
                <small>主程序 {{ report.currentMainVersion }} / 验证层 {{ report.currentAuthDllVersion }}</small>
                <div class="runtime-badges">
                  <span v-for="(value, key) in report.runtimeInfo" :key="key" class="mini-chip">
                    {{ key }}: {{ value }}
                  </span>
                </div>
              </article>
            </div>
          </section>
        </div>
      </section>
    </main>
  </div>
</template>
