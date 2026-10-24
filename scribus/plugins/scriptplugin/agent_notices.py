# -*- coding: utf-8 -*-
"""Newspaper notice ads for the Scribus AI agent.

This file is appended to agent_helpers.py when it is embedded, so everything from
agent_helpers.py (kruti_segments, kruti_font_name, ...) is available here.

An ad is a small fixed-size page: a border, a logo, a heading, an address line, a title, a
justified body that mixes Hindi (Kruti Dev) and English, and a signature block. The layout
numbers were measured from the Greater Noida notices the operators made in QuarkXPress.

Fitting works the way the operators do it: keep the point size as large as possible and
condense the letters (horizontal scale) before making the type smaller. Nothing goes below
min_pt, and never below the absolute newspaper limit of 5 pt.
"""

POINTS_PER_CM = 72.0 / 2.54
ABSOLUTE_MIN_PT = 5.0
PDF_STANDARDS = {'plain': 14, 'x1a': 11, 'x3': 12, 'x4': 10}      # values of PDFfile.version
DEFAULT_OUTPUT_PROFILE = 'ISO Coated v2 300% (basICColor)'
JUSTIFY_OVERSHOOT = 1.0105      # measured: justified, condensed text is drawn this much wider than its frame

NOTICE_TEMPLATES = {
    'greater_noida_notice': {
        'description': 'Greater Noida Industrial Development Authority public notice',
        'size_cm': (6.8, 6.5),
        'logo_file': 'greater_noida_flat.png',
        'heading_text': 'ग्रेटर नौएडा औद्योगिक विकास प्राधिकरण',
        'address_text': 'भूखण्ड संख्या–1, सैक्टर के0पी0–4 ग्रेटर नौएडा सिटी जिला–गौतमबुद्धनगर, उ0प्र0।',
        'title_text': 'सार्वजनिक सूचना',
        'signature_lines': ['प्रबन्धक (बिल्डर्स)', 'ग्रेटर नौएडा औ0 वि0 प्राधिकरण'],
        # geometry in points, measured on the 6.8 x 6.5 cm ad (192.7 x 184.2 pt)
        'border_inset': (5.81, 8.87, 6.31, 6.10),          # left, top, right, bottom
        'border_width': 0.8,
        'logo_box': (9.57, 9.82, 30.40, 32.64),             # x, y, width, height
        'text_left': 41.0,                                   # heading / address / title start here
        'heading': {'size': 20.0, 'y': 15.2},
        'address': {'size': 9.0, 'y': 37.2},
        'title': {'size': 10.0, 'y': 47.8, 'scale': 0.534},
        'body': {'x': 7.8, 'y': 59.4, 'leading_ratio': 9.5 / 9.0},
        'signature': {'size': 11.0, 'scale': 0.755, 'leading': 9.7, 'bottom_gap': 2.5, 'right_gap': 1.77,
                      'y_offset': 0.0, 'body_gap': 3.0},
    },
}


def _clean_field(value, *patterns):
    """A value as the notice wants it: surrounding words the caller may have added ('Floor', 'Sqm'...) are removed."""
    import re
    text = str(value).strip()
    for pattern in patterns:
        text = re.sub(r'(?i)(?<![A-Za-z])(?:%s)(?![A-Za-z])' % pattern, ' ', text)
    return re.sub(r'\s+', ' ', text).strip(' ,;:-.')


def _split_title(name, what):
    """Split a leading title off a person's name: returns (title for the notice, name without titles).
    Mr / Shri / Sh become 'Mr.', Dr stays 'Dr.', Late is dropped. A woman's title raises ValueError,
    because the standard wording (son of ...) is written for a man."""
    import re
    text = str(name).strip()
    title = 'Mr.'
    while True:
        match = re.match(r'(?i)^(mr|mrs|ms|miss|shri|shree|sh|smt|shrimati|dr|late)\b\.?\s*', text)
        if not match:
            break
        word = match.group(1).lower()
        if word in ('mrs', 'ms', 'miss', 'smt', 'shrimati'):
            raise ValueError('The %s %r has a title for a woman. The standard notice wording is for a man: write the text yourself.' % (what, name))
        if word == 'dr':
            title = 'Dr.'
        text = text[match.end():]
    return title, re.sub(r'\s+', ' ', text).strip()


def _ordinal(value):
    text = str(value).strip()
    if text.isdigit():
        number = int(text)
        suffix = 'th' if 10 <= number % 100 <= 20 else {1: 'st', 2: 'nd', 3: 'rd'}.get(number % 10, 'th')
        return '%d%s' % (number, suffix)
    return text


def greater_noida_mutation_body(flat, floor, tower, area, project, plot, sector, owner, address,
                                registration_date, applicant, days=35, owner_title_override=None):
    """The standard body of a Greater Noida Authority mutation notice (naamaantaran), Hindi and
    English mixed, built from the values that change from notice to notice.

    flat, floor ('12th'), tower ('D4'), area ('109.62'), project ('Nirala Aspire'), plot ('Gh-03'),
    sector ('16'), owner (the registered owner), address (the owner's address), registration_date
    ('07.09.2021'), applicant (who asks for the mutation, usually a son)."""
    owner_title, owner = _split_title(owner, 'owner')
    owner_title = owner_title if owner_title != 'Mr.' else (owner_title_override or 'Mr.')
    _, applicant = _split_title(applicant, 'applicant')
    flat = _clean_field(flat, r'flat\s*(?:no\.?)?')
    floor = _ordinal(_clean_field(floor, r'floor', r'fl\.?'))
    tower = _clean_field(tower, r'tower', r'twr\.?')
    area = _clean_field(area, r'sq\.?\s*(?:m|mt|mtr|metres?|meters?)\.?', r'sqm\.?', r'm2', r'm²')
    plot = _clean_field(plot, r'plot\s*(?:no\.?)?')
    sector = _clean_field(sector, r'sector', r'sec\.?')
    return (
        'सर्व साधारण को सूचित किया जाता है कि Flat No- %(flat)s, %(floor)s Floor, Tower- %(tower)s, '
        'Area-%(area)s Sqm., %(project)s Plot No- %(plot)s, Sector-%(sector)s, Greater Noida '
        'जो कि %(title)s %(owner)s निवासी %(address)s के नाम पर उप निबंधन कार्यालय, सदर गौतमबुद्ध नगर में '
        'दिनांक–%(date)s को पंजीकृत है। %(title)s %(owner)s की मृत्यु के उपरान्त उनके पुत्र %(applicant)s ने उक्त '
        'भूखण्ड/भवन का नामांतरण प्रस्तुत प्रपत्रों के आधार पर अपने पक्ष में करने हेतु म्यूटेशन प्रपत्र प्रस्तुत '
        'किये है। अतः इस म्यूटेशन के सम्बन्ध में यदि किसी व्यक्ति को कोई आपत्ति हो तो प्रकाशन तिथि के %(days)s '
        'दिन के अन्दर अपनी लिखित आपत्ति प्राधिकरण कार्यालय में दर्ज कराने का कष्ट करें, अन्यथा नियमानुसार '
        'प्राधिकरण अभिलेखों %(applicant)s S/o Late %(late)s %(owner)s का नाम दर्ज कर दिया जायेगा।'
    ) % {'flat': flat, 'floor': floor, 'tower': tower, 'area': area, 'project': project, 'plot': plot,
         'sector': sector, 'owner': owner, 'address': address, 'date': registration_date,
         'applicant': applicant, 'days': days, 'title': owner_title, 'late': 'Dr.' if owner_title == 'Dr.' else 'Sh.'}


def output_dir():
    """Folder where finished ads are written: SCRIBUS_AGENT_OUTPUT_DIR, else the user's Documents folder."""
    import os
    folder = os.environ.get('SCRIBUS_AGENT_OUTPUT_DIR') or os.path.join(os.path.expanduser('~'), 'Documents')
    if not os.path.isdir(folder):
        os.makedirs(folder)
    return folder


def list_notice_templates():
    return dict((name, spec['description']) for name, spec in NOTICE_TEMPLATES.items())


def find_font(family, style='Regular'):
    """Exact Scribus name of an installed font, matched by family (and style) as a substring."""
    import scribus
    names = scribus.getFontNames()
    for name in names:
        if family.lower() in name.lower() and style.lower() in name.lower():
            return name
    for name in names:
        if family.lower() in name.lower():
            return name
    return None


def _overflows(frame):
    """True when text does not fit (the whole chain of linked frames is laid out and checked)."""
    import scribus
    scribus.layoutTextChain(frame)
    last = scribus.getLastLinkedFrame(frame) or frame      # textOverflows only checks the frame it is given
    return scribus.textOverflows(last) != 0


def _put_text(frame, text, kruti, latin):
    """Fill a frame with mixed Hindi/English text: Hindi converted to Kruti Dev keystrokes and set
    in the Kruti font, everything else in the Latin font."""
    import scribus
    segments = kruti_segments(text)
    scribus.setText(''.join(seg[2] for seg in segments), frame)
    pos = 0
    for start, length, converted, is_kruti in segments:
        if converted:
            scribus.selectText(pos, len(converted), frame)
            scribus.setFont(kruti if is_kruti else latin, frame)
        pos += len(converted)
    scribus.selectText(0, 0, frame)


def _style(frame, size, scale, align, leading):
    import scribus
    scribus.setFontSize(size, frame)
    scribus.setTextScalingH(scale * 100.0, frame)
    scribus.setLineSpacingMode(0, frame)
    scribus.setLineSpacing(leading, frame)
    scribus.setTextAlignment(align, frame)
    scribus.setTextColor('Black', frame)
    scribus.setTextDistances(0, 0, 0, 0, frame)


def _largest_scale(frame, apply_scale, low, high):
    """Largest horizontal scale between low and high at which the text does not overflow, or None."""
    apply_scale(high)
    if not _overflows(frame):
        return high
    apply_scale(low)
    if _overflows(frame):
        return None
    while high - low > 0.002:
        mid = (low + high) / 2.0
        apply_scale(mid)
        if _overflows(frame):
            high = mid
        else:
            low = mid
    apply_scale(low)
    return low


def _measure_width(text, size, scale, kruti, latin):
    """Width in points of one line of text at this size and horizontal scale (found with a throw-away frame)."""
    import scribus
    height = size * 1.5
    probe = scribus.createText(0, 0, 400, height)
    _put_text(probe, text, kruti, latin)
    _style(probe, size, scale, scribus.ALIGN_LEFT, size * 1.1)
    low, high = 1.0, 400.0
    while high - low > 0.25:
        mid = (low + high) / 2.0
        scribus.sizeObject(mid, height, probe)
        if _overflows(probe):
            low = mid
        else:
            high = mid
    scribus.deleteObject(probe)
    return high


def _fit_one_line(frame, width, text, kruti, latin, max_pt, min_pt, min_scale, align):
    """One line that fills the width: the largest size that fits, condensed as far as needed.
    Returns (size, scale) or (None, None)."""
    import scribus
    size = max_pt
    while size >= min_pt - 1e-6:
        scribus.sizeObject(width, size * 1.15, frame)
        _put_text(frame, text, kruti, latin)

        def apply_scale(scale, size=size):
            _style(frame, size, scale, align, size)

        scale = _largest_scale(frame, apply_scale, min_scale, 1.0)
        if scale is not None:
            return size, scale
        size -= 0.5
    return None, None


def create_notice_ad(template, body, width_cm=None, height_cm=None, english_font='Times New Roman',
                     logo_dir=None, signature=None, max_pt=9.0, min_pt=6.0, min_scale=0.60):
    """Create a new document that holds one notice ad, and return a report.

    template     name from list_notice_templates()
    body         the notice text in Unicode, Hindi and English mixed (converted to Kruti Dev here)
    width_cm, height_cm   the ad size (default: the template's size)
    signature    optional list of lines that replaces the template's signature block
    max_pt       the largest body size to try
    min_pt       the smallest body size the fit may use (never below 5 pt)
    min_scale    the narrowest horizontal scale the fit uses before it makes the type smaller
    """
    import os
    import scribus
    if template not in NOTICE_TEMPLATES:
        raise ValueError('Unknown notice template %r. Available: %s' % (template, ', '.join(sorted(NOTICE_TEMPLATES))))
    spec = NOTICE_TEMPLATES[template]
    if min_pt < ABSOLUTE_MIN_PT:
        raise ValueError('Text smaller than %.0f pt cannot be read in a newspaper.' % ABSOLUTE_MIN_PT)
    kruti = kruti_font_name()
    if kruti is None:
        raise ValueError('The font Kruti Dev 010 is not installed.')
    latin = find_font(english_font)
    if latin is None:
        raise ValueError('The English font %r is not installed.' % english_font)
    logo_dir = logo_dir or os.environ.get('SCRIBUS_AGENT_LOGO_DIR') or ''
    logo_path = os.path.join(logo_dir, spec['logo_file'])
    if not os.path.isfile(logo_path):
        raise ValueError('Logo file not found: %s (give logo_dir or set SCRIBUS_AGENT_LOGO_DIR).' % logo_path)

    width_cm = width_cm or spec['size_cm'][0]
    height_cm = height_cm or spec['size_cm'][1]
    page_w, page_h = width_cm * POINTS_PER_CM, height_cm * POINTS_PER_CM
    report = {'template': template, 'size_cm': (width_cm, height_cm), 'warnings': []}

    scribus.newDocument((page_w, page_h), (0, 0, 0, 0), scribus.PORTRAIT, 1, scribus.UNIT_POINTS,
                        scribus.NOFACINGPAGES, scribus.FIRSTPAGERIGHT, 1)
    align_forced = getattr(scribus, 'ALIGN_FORCED', scribus.ALIGN_BLOCK)

    # border
    inset_left, inset_top, inset_right, inset_bottom = spec['border_inset']
    bx0, by0, bx1, by1 = inset_left, inset_top, page_w - inset_right, page_h - inset_bottom
    border = scribus.createRect(bx0, by0, bx1 - bx0, by1 - by0)
    scribus.setFillColor('None', border)
    scribus.setLineColor('Black', border)
    scribus.setLineWidth(spec['border_width'], border)

    # logo
    logo_x, logo_y, logo_w, logo_h = spec['logo_box']
    logo = scribus.createImage(logo_x, logo_y, logo_w, logo_h)
    scribus.loadImage(logo_path, logo)
    scribus.setScaleImageToFrame(True, True, logo)
    scribus.setFillColor('White', logo)

    # heading and address: one line each, condensed so the line fills the width
    text_left = spec['text_left']
    line_w = (bx1 - 0.98) - text_left
    for key, text_key, floor_pt in (('heading', 'heading_text', 12.0), ('address', 'address_text', min_pt)):
        frame = scribus.createText(text_left, spec[key]['y'], line_w, spec[key]['size'] * 1.15)
        size, scale = _fit_one_line(frame, line_w, spec[text_key], kruti, latin,
                                    spec[key]['size'], floor_pt, 0.35, align_forced)
        if size is None:
            raise ValueError('The %s does not fit on one line at %.0f pt or more.' % (key, floor_pt))
        report[key] = {'size': size, 'scale': round(scale, 3)}

    # title, centred
    title = scribus.createText(text_left, spec['title']['y'], line_w, spec['title']['size'] * 1.2)
    _put_text(title, spec['title_text'], kruti, latin)
    _style(title, spec['title']['size'], spec['title']['scale'], scribus.ALIGN_CENTERED, spec['title']['size'] * 1.1)

    # signature: the first line shares a row with the last, shorter row of the body (as the operators
    # lay it out by hand), the other lines sit below the body
    sig = spec['signature']
    sig_lines = list(signature or spec['signature_lines'])
    sig_widths = [_measure_width(line, sig['size'], sig['scale'], kruti, latin) + 3.0 for line in sig_lines]
    sig_right = bx1 - sig['right_gap']
    below = len(sig_lines) - 1
    body_x, body_y = spec['body']['x'], spec['body']['y']
    body_w = ((bx1 - 2.0) - body_x) / JUSTIFY_OVERSHOOT
    available = (by1 - sig['bottom_gap']) - body_y - sig['leading'] * below
    last_row_w = (sig_right - sig_widths[0] - sig['body_gap']) - body_x
    ratio = spec['body']['leading_ratio']

    # the body: a full-width frame followed by a one-row frame beside the first signature line
    body_frame = scribus.createText(body_x, body_y, body_w, 20)
    last_row = scribus.createText(body_x, body_y + 20, last_row_w, 10)
    scribus.linkTextFrames(body_frame, last_row)
    _put_text(body_frame, body, kruti, latin)

    def layout(size, scale):
        """Place the two body frames for this size; False when there is no room for two rows."""
        pitch = size * ratio
        rows = int(available / pitch + 1e-6)
        if rows < 2:
            return False
        top_h = (rows - 1) * pitch
        scribus.sizeObject(body_w, top_h, body_frame)
        scribus.moveObjectAbs(body_x, body_y, body_frame)
        scribus.sizeObject(last_row_w, pitch, last_row)
        scribus.moveObjectAbs(body_x, body_y + top_h, last_row)
        _style(body_frame, size, scale, scribus.ALIGN_BLOCK, pitch)
        report['rows'] = rows
        return True

    def body_overflows(size, scale):
        return (not layout(size, scale)) or _overflows(body_frame)

    def best_scale(size, low, high):
        if not body_overflows(size, high):
            return high
        if body_overflows(size, low):
            return None
        while high - low > 0.002:
            mid = (low + high) / 2.0
            if body_overflows(size, mid):
                high = mid
            else:
                low = mid
        body_overflows(size, low)
        return low

    chosen = None
    size = max_pt
    while size >= min_pt - 1e-6:
        scale = best_scale(size, min_scale, 1.0)
        if scale is not None:
            chosen = (size, scale)
            break
        size -= 0.5
    if chosen is None:
        # still too long: condense further at the smallest allowed size before giving up
        scale = best_scale(min_pt, 0.5, min_scale)
        if scale is not None:
            chosen = (min_pt, scale)
            report['warnings'].append('The text only fits at %.1f pt squeezed to %d%%. Check that it is still readable.'
                                      % (min_pt, round(scale * 100)))
    if chosen is None:
        body_overflows(min_pt, min_scale)
        report['fits'] = False
        report['warnings'].append('The text does not fit this ad size even at %.1f pt. Use a larger ad or shorten the text.' % min_pt)
    else:
        report['fits'] = True
        report['body'] = {'size': chosen[0], 'scale': round(chosen[1], 3), 'rows': report.get('rows')}

    # signature frames (placed after the fit, because the body decides where its last row is)
    pitch = (chosen[0] if chosen else min_pt) * ratio
    last_row_y = body_y + (report.get('rows', 2) - 1) * pitch
    sig_frames = []
    for index, line in enumerate(sig_lines):
        frame_y = last_row_y + sig['y_offset'] + index * sig['leading']
        frame = scribus.createText(sig_right - sig_widths[index], frame_y, sig_widths[index], sig['size'] * 1.5)
        _put_text(frame, line, kruti, latin)
        # Scribus mis-places right-aligned condensed text, so the frame is sized to the line and forced-justified
        _style(frame, sig['size'], sig['scale'], align_forced, sig['leading'])
        sig_frames.append(frame)
    report['signature_lines'] = len(sig_lines)
    sig_frame = sig_frames[0]
    report['frames'] = {'border': border, 'logo': logo, 'body': body_frame, 'signature': sig_frame, 'title': title}
    return report


def _put_paragraphs(frame, paragraphs, kruti, latin):
    """Fill a frame with several paragraphs of mixed Hindi/English text; returns [(start, length)] per paragraph."""
    import scribus
    pieces, ranges, runs = [], [], []
    pos = 0
    for text in paragraphs:
        converted_total = 0
        for start, length, converted, is_kruti in kruti_segments(text):
            if converted:
                runs.append((pos + converted_total, len(converted), kruti if is_kruti else latin))
            converted_total += len(converted)
            pieces.append(converted)
        ranges.append((pos, converted_total))
        pos += converted_total + 1
        pieces.append(chr(10))
    scribus.setText(''.join(pieces[:-1]), frame)
    for start, length, font in runs:
        scribus.selectText(start, length, frame)
        scribus.setFont(font, frame)
    scribus.selectText(0, 0, frame)
    return ranges


COLUMN_ALIGNS = {'center': 'ALIGN_CENTERED', 'left': 'ALIGN_LEFT', 'right': 'ALIGN_RIGHT', 'justify': 'ALIGN_BLOCK'}


def create_column_notice(paragraphs, width_cm=3.5, height_cm=10.5, english_font='Times New Roman',
                         max_pt=8.0, min_pt=6.0, min_scale=0.60, border_width=0.8):
    """Create a new document with a bordered notice for a narrow newspaper column (court and
    tribunal notices, summons, legal notices) and return a report.

    paragraphs   list of (text, align) or (text, align, size_factor). text is Unicode, Hindi and English
                 mixed (converted to Kruti Dev here). align is 'center', 'left', 'right' or 'justify'.
                 size_factor makes a paragraph larger than the body (a court heading is about 1.2).
    width_cm, height_cm   the ad size (a column is 3.5 cm wide)
    max_pt       the largest body size to try
    min_pt       the smallest body size the fit may use (never below 5 pt)
    min_scale    the narrowest horizontal scale the fit uses before it makes the type smaller

    Bold and italic are not applied: Kruti Dev has no styled variants installed.
    """
    import scribus
    if min_pt < ABSOLUTE_MIN_PT:
        raise ValueError('Text smaller than %.0f pt cannot be read in a newspaper.' % ABSOLUTE_MIN_PT)
    kruti = kruti_font_name()
    if kruti is None:
        raise ValueError('The font Kruti Dev 010 is not installed.')
    latin = find_font(english_font)
    if latin is None:
        raise ValueError('The English font %r is not installed.' % english_font)
    specs = []
    for item in paragraphs:
        text, align = item[0], item[1]
        if align not in COLUMN_ALIGNS:
            raise ValueError('align must be one of %s, not %r' % (', '.join(sorted(COLUMN_ALIGNS)), align))
        specs.append((text, getattr(scribus, COLUMN_ALIGNS[align]), item[2] if len(item) > 2 else 1.0))
    if not specs:
        raise ValueError('No paragraphs given.')

    page_w, page_h = width_cm * POINTS_PER_CM, height_cm * POINTS_PER_CM
    report = {'size_cm': (width_cm, height_cm), 'warnings': []}
    scribus.newDocument((page_w, page_h), (0, 0, 0, 0), scribus.PORTRAIT, 1, scribus.UNIT_POINTS,
                        scribus.NOFACINGPAGES, scribus.FIRSTPAGERIGHT, 1)
    inset = 2.0
    border = scribus.createRect(inset, inset, page_w - 2 * inset, page_h - 2 * inset)
    scribus.setFillColor('None', border)
    scribus.setLineColor('Black', border)
    scribus.setLineWidth(border_width, border)

    pad = 2.5
    frame = scribus.createText(inset + pad, inset + pad, (page_w - 2 * inset - 2 * pad) / JUSTIFY_OVERSHOOT,
                               page_h - 2 * inset - 2 * pad)
    ranges = _put_paragraphs(frame, [spec[0] for spec in specs], kruti, latin)

    def apply(size, scale):
        scribus.setTextScalingH(scale * 100.0, frame)
        scribus.setLineSpacingMode(0, frame)
        scribus.setTextColor('Black', frame)
        scribus.setTextDistances(0, 0, 0, 0, frame)
        for (start, length), (text, align, factor) in zip(ranges, specs):
            if length == 0:
                continue
            scribus.selectText(start, length, frame)
            scribus.setFontSize(size * factor, frame)
            scribus.setLineSpacing(size * factor * 1.08, frame)
            scribus.setTextAlignment(align, frame)
        scribus.selectText(0, 0, frame)

    def fits(size, scale):
        apply(size, scale)
        scribus.layoutTextChain(frame)
        return not _overflows(frame)

    chosen = None
    size = max_pt
    while size >= min_pt - 1e-6 and chosen is None:
        if fits(size, 1.0):
            chosen = (size, 1.0)
        elif fits(size, min_scale):
            low, high = min_scale, 1.0
            while high - low > 0.002:
                mid = (low + high) / 2.0
                if fits(size, mid):
                    low = mid
                else:
                    high = mid
            fits(size, low)
            chosen = (size, low)
        size -= 0.5
    if chosen is None:
        low, high = 0.5, min_scale
        if fits(min_pt, low):
            while high - low > 0.002:
                mid = (low + high) / 2.0
                if fits(min_pt, mid):
                    low = mid
                else:
                    high = mid
            fits(min_pt, low)
            chosen = (min_pt, low)
            report['warnings'].append('The text only fits at %.1f pt squeezed to %d%%. Check that it is still readable.'
                                      % (min_pt, round(low * 100)))
    if chosen is None:
        fits(min_pt, min_scale)
        report['fits'] = False
        report['warnings'].append('The text does not fit this ad size even at %.1f pt. Use a taller ad or shorten the text.' % min_pt)
    else:
        report['fits'] = True
        report['body'] = {'size': chosen[0], 'scale': round(chosen[1], 3)}
    report['frames'] = {'border': border, 'text': frame}
    return report


def export_ad_pdf(path, standard='x1a', profile=None, title='Notice ad'):
    """Export the open ad as PDF. standard: 'plain', 'x1a', 'x3' or 'x4' (PDF/X-1a, X-3, X-4)."""
    import os
    import scribus
    if standard not in PDF_STANDARDS:
        raise ValueError('standard must be one of %s' % ', '.join(sorted(PDF_STANDARDS)))
    path = os.path.expanduser(path)                          # '~/Desktop/ad.pdf' works
    if not os.path.dirname(path):
        path = os.path.join(output_dir(), path)               # a bare file name goes to the output folder
    path = os.path.abspath(path)
    if not path.lower().endswith('.pdf'):
        path += '.pdf'
    scribus.setInfo('Scribus AI agent', title, '')
    pdf = scribus.PDFfile()
    pdf.file = path
    pdf.version = PDF_STANDARDS[standard]
    pdf.fontEmbedding = 0
    pdf.compress = 1
    pdf.quality = 0
    if standard != 'plain':
        pdf.outdst = 1
        pdf.info = title
        pdf.printprofc = profile or DEFAULT_OUTPUT_PROFILE
    pdf.save()
    print('PDF saved to: ' + path)
    return path
