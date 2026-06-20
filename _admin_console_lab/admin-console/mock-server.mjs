import crypto from 'node:crypto'
import express from 'express'
import cors from 'cors'
import multer from 'multer'

const app = express()
const upload = multer({ storage: multer.memoryStorage() })
const PORT = 30186

app.use(cors())
app.use(express.json({ limit: '64mb' }))

const nowIso = () => new Date().toISOString()
const hex = (input, algorithm) => crypto.createHash(algorithm).update(input).digest('hex')
const qqAvatar = (qq) => `https://q1.qlogo.cn/g?b=qq&nk=${encodeURIComponent(String(qq))}&s=100`

function detectVersion(fileName = '') {
  const match = String(fileName).match(/v?(\d+\.\d+\.\d+(?:[._-]\d+)?)/i)
  if (!match) {
    return { version: '', versionCode: 0 }
  }
  const normalized = match[1].replace(/_/g, '.')
  const parts = normalized.split(/[.-]/).map((item) => Number(item) || 0)
  const [major = 0, minor = 0, patch = 0, build = 0] = parts
  return {
    version: `v${normalized}`,
    versionCode: major * 1_000_000 + minor * 10_000 + patch * 100 + build,
  }
}

const state = {
  settings: {
    downloadMode: 'server-url',
  },
  artifacts: [
    {
      category: 'bootstrap',
      artifactType: 'auth_dll',
      fileName: 'NoteBotAuth_3.4.58.dll',
      version: '3.4.58',
      versionCode: 30458,
      enabled: true,
      required: true,
      protocolMin: 3,
      protocolMax: 3,
      size: 12646400,
      md5: 'b9968251aeba0e9fa97fa49e5fb461bc',
      sha256: 'ce45265d01dca0e8a5591abee30c7ebfda4128ba449b2f7b7b9d1f8b164f498d',
      publishedAtUtc: nowIso(),
      uploadSource: 'seed',
    },
    {
      category: 'notebot',
      artifactType: 'overlay_dll',
      fileName: 'NoteBot_v6.4.75_20260602_1353.dll',
      version: 'v6.4.75.20260602',
      versionCode: 6047502,
      enabled: true,
      required: false,
      protocolMin: 3,
      protocolMax: 3,
      size: 1400832,
      md5: '0ad9e6e2bfd0f1e01158462815349dfb',
      sha256: 'b746b604962ad0ce367a53257953b4b09dbe23351d0421430ed2a6b0c4e62021',
      publishedAtUtc: nowIso(),
      uploadSource: 'seed',
    },
    {
      category: 'bootstrap',
      artifactType: 'updater_exe',
      fileName: 'NoteBotUpdater_3.4.76.exe',
      version: '3.4.76',
      versionCode: 30476,
      enabled: true,
      required: false,
      protocolMin: 3,
      protocolMax: 3,
      size: 7403520,
      md5: 'seed-updater-md5',
      sha256: 'seed-updater-sha256',
      publishedAtUtc: nowIso(),
      uploadSource: 'seed',
    },
  ],
  licenses: [
    {
      keyId: 'kid_0231682ae89e0ca3',
      keyPlaintextNormalized: 'NB-2E686994-E48206F3',
      tier: 'Dev',
      tierValue: 3,
      featureFlags: 0xffffffff,
      revoked: false,
      maxDevices: 1,
      boundQQ: '498679454',
      nickname: 'William',
      avatarUrl: qqAvatar('498679454'),
      boundDllName: 'NoteBot_v6.4.75_20260602_1353.dll',
      boundDllSha256: 'b746b604962ad0ce367a53257953b4b09dbe23351d0421430ed2a6b0c4e62021',
      durationDays: 0,
      activatedAtUtc: '2026-05-19T13:00:12.000Z',
      expiresAtUtc: null,
      updatedAtUtc: nowIso(),
      note: '核心测试账户',
    },
    {
      keyId: 'kid_78fdaf170e9576dc',
      keyPlaintextNormalized: 'NB-E3174CF6-B1F5903D',
      tier: 'Premium',
      tierValue: 2,
      featureFlags: 16,
      revoked: false,
      maxDevices: 2,
      boundQQ: '2815764628',
      nickname: '白名单用户',
      avatarUrl: qqAvatar('2815764628'),
      boundDllName: 'NoteBot_v6.4.75_20260602_1353.dll',
      boundDllSha256: 'b746b604962ad0ce367a53257953b4b09dbe23351d0421430ed2a6b0c4e62021',
      durationDays: 30,
      activatedAtUtc: '2026-06-02T09:07:02.000Z',
      expiresAtUtc: '2026-07-02T09:07:02.000Z',
      updatedAtUtc: nowIso(),
      note: '有自定义名称权限',
    },
    {
      keyId: 'kid_cdb60912674ae845',
      keyPlaintextNormalized: 'NB-D1E840EE-1FE4EA39',
      tier: 'Trial',
      tierValue: 1,
      featureFlags: 0,
      revoked: false,
      maxDevices: 1,
      boundQQ: '3615915223',
      nickname: '新用户观察中',
      avatarUrl: qqAvatar('3615915223'),
      boundDllName: 'NoteBot_v6.4.73_20260602_0513.dll',
      boundDllSha256: '5ef16e2b76caa87171cadcb28136f8ae6979b9f09dca04a3793fad0469d02c75',
      durationDays: 7,
      activatedAtUtc: null,
      expiresAtUtc: null,
      updatedAtUtc: nowIso(),
      note: '还没有真正上机',
    },
  ],
  devices: [
    {
      keyId: 'kid_0231682ae89e0ca3',
      keyPlaintextNormalized: 'NB-2E686994-E48206F3',
      deviceId: 'did_2ef817ca9a6b4f20',
      machineLabel: 'William-PC',
      status: 'online',
      firstActivatedAtUtc: '2026-05-19T13:00:12.000Z',
      lastSeenAtUtc: nowIso(),
    },
    {
      keyId: 'kid_78fdaf170e9576dc',
      keyPlaintextNormalized: 'NB-E3174CF6-B1F5903D',
      deviceId: 'did_8bf0c10ac91d245f',
      machineLabel: 'Studio-Laptop',
      status: 'cooldown',
      firstActivatedAtUtc: '2026-06-02T09:07:02.000Z',
      lastSeenAtUtc: '2026-06-05T03:13:00.000Z',
    },
  ],
  events: [
    { id: crypto.randomUUID(), tsUtc: nowIso(), category: 'system', action: 'server_v3', status: 'ok', message: '本地原型后端已启动' },
    { id: crypto.randomUUID(), tsUtc: nowIso(), category: 'upload', action: 'auth_dll', status: 'idle', message: '等待上传新的验证层 DLL' },
  ],
  runtimeReports: [
    {
      id: crypto.randomUUID(),
      receivedAtUtc: nowIso(),
      keyId: 'kid_0231682ae89e0ca3',
      deviceId: 'did_2ef817ca9a6b4f20',
      clientKind: 'NoteBot Injector',
      channel: 'stable',
      currentMainVersion: '3.4.76',
      currentAuthDllVersion: '3.4.58',
      currentUpdaterVersion: '3.4.76',
      protocolVersion: 3,
      runtimeInfo: { os: 'Windows 11', cpu: 'i7-13700K', gpu: 'RTX 4070', memoryGb: 32 },
    },
  ],
}

const pushEvent = (category, action, status, message) => {
  state.events.unshift({
    id: crypto.randomUUID(),
    tsUtc: nowIso(),
    category,
    action,
    status,
    message,
  })
  state.events = state.events.slice(0, 24)
}

const getDeviceCount = (keyId) => state.devices.filter((item) => item.keyId === keyId).length

const overview = () => ({
  service: 'admin_console_lab',
  protocolVersion: 3,
  abiVersion: 1,
  counts: {
    licenses: state.licenses.length,
    onlineDevices: state.devices.filter((item) => item.status === 'online').length,
    alerts: state.licenses.filter((item) => item.revoked || !item.activatedAtUtc).length,
    artifacts: state.artifacts.length,
  },
  latestArtifacts: {
    authDll: state.artifacts.find((item) => item.artifactType === 'auth_dll') ?? null,
    overlayDll: state.artifacts.find((item) => item.artifactType === 'overlay_dll') ?? null,
    updaterExe: state.artifacts.find((item) => item.artifactType === 'updater_exe') ?? null,
  },
  settings: state.settings,
})

app.get('/api/overview', (_req, res) => {
  res.json({ status: 'ok', data: overview() })
})

app.get('/api/licenses', (_req, res) => {
  res.json({
    status: 'ok',
    data: state.licenses.map((item) => ({
      ...item,
      deviceCount: getDeviceCount(item.keyId),
    })),
  })
})

app.post('/api/licenses', (req, res) => {
  const normalized = String(req.body?.keyPlaintextNormalized || '').trim().toUpperCase()
  if (!normalized) {
    return res.status(400).json({ status: 'error', message: 'key_required' })
  }
  if (state.licenses.some((item) => item.keyPlaintextNormalized === normalized)) {
    return res.status(409).json({ status: 'error', message: 'key_exists' })
  }
  const keyHash = hex(normalized, 'sha256')
  const tier = String(req.body?.tier || 'Premium')
  const tierValue = { Trial: 1, Premium: 2, Dev: 3 }[tier] ?? 2
  const qq = String(req.body?.boundQQ || '')
  const created = {
    keyId: `kid_${keyHash.slice(0, 16)}`,
    keyPlaintextNormalized: normalized,
    tier,
    tierValue,
    featureFlags: Number(req.body?.featureFlags || 0),
    revoked: false,
    maxDevices: Number(req.body?.maxDevices || 1),
    boundQQ: qq,
    nickname: qq ? `QQ ${qq}` : '未命名用户',
    avatarUrl: qq ? qqAvatar(qq) : '',
    boundDllName: String(req.body?.boundDllName || ''),
    boundDllSha256: String(req.body?.boundDllSha256 || ''),
    durationDays: Number(req.body?.durationDays || 30),
    activatedAtUtc: req.body?.activatedAtUtc || null,
    expiresAtUtc: req.body?.expiresAtUtc || null,
    updatedAtUtc: nowIso(),
    note: String(req.body?.note || ''),
  }
  state.licenses.unshift(created)
  pushEvent('license', 'create', 'ok', `创建密钥 ${normalized}`)
  res.json({ status: 'ok', data: created })
})

app.patch('/api/licenses/:keyId', (req, res) => {
  const target = state.licenses.find((item) => item.keyId === req.params.keyId)
  if (!target) {
    return res.status(404).json({ status: 'error', message: 'license_not_found' })
  }
  Object.assign(target, {
    tier: req.body?.tier ?? target.tier,
    tierValue: req.body?.tierValue ?? target.tierValue,
    boundQQ: req.body?.boundQQ ?? target.boundQQ,
    nickname: req.body?.nickname ?? target.nickname,
    featureFlags: req.body?.featureFlags ?? target.featureFlags,
    maxDevices: req.body?.maxDevices ?? target.maxDevices,
    revoked: req.body?.revoked ?? target.revoked,
    boundDllName: req.body?.boundDllName ?? target.boundDllName,
    expiresAtUtc: req.body?.expiresAtUtc ?? target.expiresAtUtc,
    note: req.body?.note ?? target.note,
    updatedAtUtc: nowIso(),
  })
  if (target.boundQQ) {
    target.avatarUrl = qqAvatar(target.boundQQ)
  }
  pushEvent('license', 'update', 'ok', `更新密钥 ${target.keyPlaintextNormalized}`)
  res.json({ status: 'ok', data: target })
})

app.delete('/api/licenses/:keyId', (req, res) => {
  const idx = state.licenses.findIndex((item) => item.keyId === req.params.keyId)
  if (idx === -1) {
    return res.status(404).json({ status: 'error', message: 'license_not_found' })
  }
  const removed = state.licenses.splice(idx, 1)[0]
  state.devices = state.devices.filter((item) => item.keyId !== removed.keyId)
  pushEvent('license', 'delete', 'ok', `删除密钥 ${removed.keyPlaintextNormalized}`)
  res.json({ status: 'ok' })
})

app.post('/api/licenses/batch-bind-overlay', (req, res) => {
  const targetDll = String(req.body?.boundDllName || '')
  const tier = String(req.body?.tier || '')
  state.licenses.forEach((item) => {
    if (!tier || item.tier === tier) {
      item.boundDllName = targetDll
      item.updatedAtUtc = nowIso()
    }
  })
  pushEvent('license', 'batch_bind_overlay', 'ok', `批量绑定 ${targetDll}`)
  res.json({ status: 'ok' })
})

app.get('/api/devices', (_req, res) => {
  res.json({ status: 'ok', data: state.devices })
})

app.delete('/api/devices/:keyId/:deviceId', (req, res) => {
  state.devices = state.devices.filter(
    (item) => !(item.keyId === req.params.keyId && item.deviceId === req.params.deviceId),
  )
  pushEvent('device', 'remove', 'ok', `移除设备 ${req.params.deviceId}`)
  res.json({ status: 'ok' })
})

app.post('/api/devices/:keyId/reset', (req, res) => {
  state.devices = state.devices.filter((item) => item.keyId !== req.params.keyId)
  pushEvent('device', 'reset', 'ok', `重置密钥 ${req.params.keyId} 的全部设备`)
  res.json({ status: 'ok' })
})

app.get('/api/artifacts', (_req, res) => {
  res.json({ status: 'ok', data: state.artifacts })
})

app.patch('/api/artifacts/:sha256/state', (req, res) => {
  const target = state.artifacts.find((item) => item.sha256 === req.params.sha256)
  if (!target) {
    return res.status(404).json({ status: 'error', message: 'artifact_not_found' })
  }
  target.enabled = Boolean(req.body?.enabled)
  pushEvent('artifact', 'toggle', 'ok', `${target.fileName} 已${target.enabled ? '启用' : '停用'}`)
  res.json({ status: 'ok', data: target })
})

app.post('/api/artifacts/upload', upload.single('file'), async (req, res) => {
  if (!req.file) {
    return res.status(400).json({ status: 'error', message: 'file_required' })
  }
  await new Promise((resolve) => setTimeout(resolve, 420))
  const category = String(req.body?.category || 'notebot')
  const artifactType = String(req.body?.artifactType || 'overlay_dll')
  const detected = detectVersion(req.file.originalname)
  const version = String(req.body?.version || detected.version || '')
  const versionCode = Number(req.body?.versionCode || detected.versionCode || 0)
  const sha256 = hex(req.file.buffer, 'sha256')
  const artifact = {
    category,
    artifactType,
    fileName: req.file.originalname,
    version,
    versionCode,
    enabled: true,
    required: artifactType === 'auth_dll',
    protocolMin: Number(req.body?.protocolMin || 3),
    protocolMax: Number(req.body?.protocolMax || 3),
    size: req.file.size,
    md5: hex(req.file.buffer, 'md5'),
    sha256,
    publishedAtUtc: nowIso(),
    uploadSource: 'local_mock_upload',
  }
  state.artifacts.unshift(artifact)
  pushEvent('artifact', 'upload', 'ok', `上传 ${artifact.fileName}`)
  res.json({ status: 'ok', data: artifact })
})

app.get('/api/events', (_req, res) => {
  res.json({ status: 'ok', data: state.events })
})

app.get('/api/runtime-reports', (_req, res) => {
  res.json({ status: 'ok', data: state.runtimeReports })
})

app.patch('/api/settings/download-mode', (req, res) => {
  state.settings.downloadMode = String(req.body?.downloadMode || state.settings.downloadMode)
  pushEvent('settings', 'download_mode', 'ok', `下载模式切换为 ${state.settings.downloadMode}`)
  res.json({ status: 'ok', data: state.settings })
})

app.listen(PORT, '127.0.0.1', () => {
  console.log(`[mock-api] listening on http://127.0.0.1:${PORT}`)
})
