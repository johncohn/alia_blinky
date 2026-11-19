/**
 * "Your own sparkly Alia" sign-ups -> Google Sheet + emails
 *
 * Paste this into Extensions -> Apps Script of the Google Sheet that should
 * collect sign-ups, fill in CONTACT_* below, then deploy it as a web app
 * (see docs/SETUP.md).
 *
 * What it does:
 *   - Each sign-up from the page adds a row to the "Signups" tab
 *   - Emails the buyer a confirmation: order, total, pickup info, your
 *     contact details, and a Venmo button + QR code ("pay now or when your
 *     Alia is delivered")
 *   - Emails you (the sheet owner) about each sign-up
 *   - When you tick "Paid?" on a row, emails that buyer a payment
 *     confirmation (once - the time it was sent goes in "Paid email sent")
 */

// ===== FILL THESE IN =====
const CONTACT_NAME = 'John Cohn';
const CONTACT_EMAIL = 'you@example.com';      // Shown in emails; replies go here
const CONTACT_PHONE = '555-555-5555';
const VENMO_USER = 'johncohnvt';
const PAGE_URL = 'https://johncohn.github.io/alia_blinky/';  // Venmo QR image is loaded from here
const PRICE = 30;
const PICKUP_TEXT = 'Pick up at Beta the week of December 14 (or we\'ll mail it to the address you gave us).';
const NOTIFY_OWNER = true;                   // Email you about every sign-up

// ===== SHEET LAYOUT =====
const SHEET_NAME = 'Signups';
const HEADERS = ['Timestamp', 'Name', 'Email', 'Phone', 'Quantity', 'Total ($)', 'Message', 'Paid?', 'Paid email sent'];
const COL = { name: 2, email: 3, qty: 5, total: 6, paid: 8, paidSent: 9 };

// ---------------------------------------------------------------------------
// Form submissions from the page
// ---------------------------------------------------------------------------
function doPost(e) {
  const p = (e && e.parameter) || {};
  if (p.website) return reply_();  // Honeypot field filled in -> a bot, ignore

  const lock = LockService.getScriptLock();
  lock.waitLock(10000);
  let row;
  try {
    const sheet = getSheet_();
    const qty = Math.max(0, Math.min(5, parseInt(p.quantity, 10) || 0));
    row = [new Date(), clean_(p.name), clean_(p.email), clean_(p.phone),
           qty, qty * PRICE, clean_(p.message), false, ''];
    sheet.appendRow(row);
    const r = sheet.getLastRow();
    if (qty > 0) sheet.getRange(r, COL.paid).insertCheckboxes();
  } finally {
    lock.releaseLock();
  }

  const [, name, email, phone, qty, total, message] = row;
  try {
    if (/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) sendConfirmation_(name, email, qty, total);
  } catch (err) {
    console.error('Confirmation email failed: ' + err);
  }
  if (NOTIFY_OWNER) {
    const subject = qty > 0 ? `Sparkly Alia: ${qty} for ${name} ($${total})` : `Sparkly Alia question from ${name}`;
    MailApp.sendEmail(Session.getEffectiveUser().getEmail(), subject,
      `Name: ${name}\nEmail: ${email}\nPhone: ${phone}\nQuantity: ${qty}\nTotal: $${total}\n\nMessage:\n${message || '(none)'}`);
  }
  return reply_();
}

// ---------------------------------------------------------------------------
// Emails
// ---------------------------------------------------------------------------
function sendConfirmation_(name, email, qty, total) {
  const first = esc_(String(name).split(' ')[0] || 'there');
  let subject, body;
  if (qty > 0) {
    const venmo = venmoLink_(total);
    subject = `You're on the list for ${qty} sparkly Alia${qty > 1 ? 's' : ''}! ✨`;
    body = `
      <p>Hi ${first},</p>
      <p>Thanks for signing up! Here's your order:</p>
      <p style="font-size:18px"><b>${qty} × sparkly Alia · $${total}</b></p>
      <p>${esc_(PICKUP_TEXT)}</p>
      <h3 style="margin-bottom:4px">Paying</h3>
      <p style="margin-top:0">You can <b>pay now</b> with Venmo, or <b>pay when your Alia is delivered</b>, whichever you like.</p>
      <p><a href="${venmo}" style="display:inline-block;background:#3d95ce;color:#fff;padding:12px 22px;border-radius:10px;text-decoration:none;font-weight:bold">Pay $${total} with Venmo</a></p>
      <p>Or scan this with your phone (Venmo @${esc_(VENMO_USER)}):<br>
        <img src="${PAGE_URL}venmo-qr.png" alt="Venmo QR code" width="180" height="180" style="margin-top:8px"></p>
      <p>🎁 <b>200% goes to Toys for Tots</b>, because every purchase is matched. Thank you!</p>
      ${contactBlock_()}`;
  } else {
    subject = 'Thanks for your message about the sparkly Alia ✨';
    body = `
      <p>Hi ${first},</p>
      <p>Thanks for your message! We'll get back to you soon.</p>
      ${contactBlock_()}`;
  }
  send_(email, subject, body);
}

function sendPaymentConfirmation_(name, email, qty, total) {
  const first = esc_(String(name).split(' ')[0] || 'there');
  send_(email, 'Payment received, thank you! 🎉', `
    <p>Hi ${first},</p>
    <p>We've received your payment of <b>$${total}</b> for <b>${qty} sparkly Alia${qty > 1 ? 's' : ''}</b>. Thank you!</p>
    <p>${esc_(PICKUP_TEXT)}</p>
    <p>🎁 Your purchase is matched, so <b>$${total * 2}</b> goes to Toys for Tots.</p>
    ${contactBlock_()}`);
}

function contactBlock_() {
  return `<p style="color:#555;border-top:1px solid #ddd;padding-top:10px;margin-top:20px">
    Questions? Contact ${esc_(CONTACT_NAME)}<br>
    ✉️ <a href="mailto:${CONTACT_EMAIL}">${esc_(CONTACT_EMAIL)}</a> · 📞 ${esc_(CONTACT_PHONE)}</p>`;
}

function send_(to, subject, htmlBody) {
  MailApp.sendEmail({
    to: to, subject: subject, name: 'Sparkly Alia', replyTo: CONTACT_EMAIL,
    htmlBody: `<div style="font-family:Arial,sans-serif;font-size:15px;line-height:1.5;color:#222;max-width:520px">${htmlBody}</div>`,
  });
}

function venmoLink_(amount) {
  // Venmo's payment page with you as the recipient and the amount filled in
  return 'https://account.venmo.com/pay?txn=pay&recipients=' + encodeURIComponent(VENMO_USER) +
         '&amount=' + amount + '&note=' + encodeURIComponent('Sparkly Alia');
}

// ---------------------------------------------------------------------------
// "Paid?" checkbox -> payment confirmation email
// ---------------------------------------------------------------------------
// Adds a "Sparkly Alia" menu to the sheet. Use it once to turn on payment emails.
function onOpen() {
  SpreadsheetApp.getUi().createMenu('Sparkly Alia')
    .addItem('Turn on payment emails', 'installPaidTrigger')
    .addToUi();
}

// Run once (from the menu). Ticking "Paid?" then sends the payment email.
function installPaidTrigger() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  ScriptApp.getProjectTriggers()
    .filter(t => t.getHandlerFunction() === 'onPaidEdit')
    .forEach(t => ScriptApp.deleteTrigger(t));
  ScriptApp.newTrigger('onPaidEdit').forSpreadsheet(ss).onEdit().create();
  getSheet_();
  SpreadsheetApp.getUi().alert('Payment emails are on. Tick "Paid?" on a row to email that person a payment confirmation.');
}

function onPaidEdit(e) {
  const range = e.range;
  const sheet = range.getSheet();
  if (sheet.getName() !== SHEET_NAME || range.getColumn() !== COL.paid || range.getRow() < 2) return;
  if (range.getValue() !== true) return;

  const r = range.getRow();
  const vals = sheet.getRange(r, 1, 1, HEADERS.length).getValues()[0];
  if (vals[COL.paidSent - 1]) return;  // Already sent
  const name = vals[COL.name - 1], email = vals[COL.email - 1];
  const qty = vals[COL.qty - 1], total = vals[COL.total - 1];
  if (!email || !qty) return;

  sendPaymentConfirmation_(name, email, qty, total);
  sheet.getRange(r, COL.paidSent).setValue(new Date());
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
function getSheet_() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const sheet = ss.getSheetByName(SHEET_NAME) || ss.insertSheet(SHEET_NAME);
  if (sheet.getLastRow() === 0) {
    sheet.appendRow(HEADERS);
    sheet.setFrozenRows(1);
    sheet.getRange(1, 1, 1, HEADERS.length).setFontWeight('bold');
  }
  return sheet;
}

// Trim, cap the length, and stop text being treated as a spreadsheet formula
function clean_(v) {
  let s = String(v || '').trim().slice(0, 2000);
  if (/^[=+\-@]/.test(s)) s = "'" + s;
  return s;
}

function esc_(s) {
  return String(s).replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

function reply_() {
  return ContentService.createTextOutput(JSON.stringify({ ok: true }))
    .setMimeType(ContentService.MimeType.JSON);
}
